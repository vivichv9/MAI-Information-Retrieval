use crate::config;
use crate::models::Urls;

use anyhow::Result;
use chrono::DateTime;
use chrono::offset::Utc;
use diesel::prelude::*;
use html5ever::driver::{ParseOpts, parse_document};
use html5ever::local_name;
use html5ever::tendril::TendrilSink;
use markup5ever_rcdom::{Handle, NodeData, RcDom};
use mongodb::bson::doc;
use std::time::SystemTime;
use std::{thread, time};

pub struct Crawler {
    config: config::Config,
    pub disallow: Vec<String>,
}

fn collect_all_hrefs(node: &Handle, out: &mut Vec<String>) {
    if let NodeData::Element { attrs, .. } = &node.data {
        for attr in attrs.borrow().iter() {
            if attr.name.local == local_name!("href") {
                out.push(attr.value.to_string());
            }
        }
    }

    for child in node.children.borrow().iter() {
        collect_all_hrefs(child, out);
    }
}

impl Crawler {
    pub fn new(config: config::Config) -> Self {
        Self {
            config,
            disallow: Vec::new(),
        }
    }

    fn get_next_url(&mut self) -> String {
        use crate::schema::urls::dsl::*;
        let conn = &mut self.config.metadata_db;
        let results = urls
            .filter(is_parsed.eq(false))
            .limit(1)
            .select(Urls::as_select())
            .load(conn)
            .expect("Error loading urls");

        if results.is_empty() {
            return String::from(&self.config.domain) + &self.config.initial_path;
        }

        results.into_iter().next().unwrap().path
    }

    pub fn get_document(&mut self, url: &str) -> Result<String> {

        // for banned in self.disallow.iter() {
        //     if url.starts_with(banned) {
        //         return Result::Err(Error::from(""));
        //     }
        // }

        let response = reqwest::blocking::get(url)?;
        Ok(response.text()?)
    }

    pub fn update_metadata(&mut self, doc: &str) -> Result<()> {
        use crate::schema::urls::dsl::*;

        let dom: RcDom = parse_document(RcDom::default(), ParseOpts::default()).one(doc);
        let mut out = Vec::new();
        collect_all_hrefs(&dom.document, &mut out);
        let received: Vec<Urls> = out
            .into_iter()
            .filter(|x| x.starts_with(&self.config.domain))
            .map(|x| Urls {
                path: x,
                is_parsed: false,
            })
            .collect();

        diesel::insert_into(urls)
            .values(received)
            .on_conflict_do_nothing()
            .execute(&mut self.config.metadata_db)?;

        Ok(())
    }

    pub fn update_document(&mut self, doc: &str, href: &str) -> Result<()> {
        use crate::schema::urls::dsl::*;

        let time = SystemTime::now();
        let datetime: DateTime<Utc> = time.into();

        diesel::update(urls)
            .filter(path.eq(href))
            .set(is_parsed.eq(true))
            .execute(&mut self.config.metadata_db)?;

        let insert_doc = doc! {
            "text": doc.to_string(),
            "url": href.to_string(),
            "parsed_dttm": datetime.format("%d/%m/%Y %T").to_string(),
        };

        self.config.collection.insert_one(insert_doc).run()?;
        Ok(())
    }

    #[must_use]
    fn get_scraping_policy(&mut self) -> Result<()> {
        let robots_path = self.config.domain.clone() + "robots.txt";
        let response = reqwest::blocking::get(robots_path)?;
        let data = response.text()?;

        let mut current_user_agent = None;
        let mut in_target_section = false;
        for line in data.lines() {
            let line = line.trim();

            if line.is_empty() {
                continue;
            }

            if line.starts_with('#') {
                continue;
            }

            if line.to_lowercase().starts_with("user-agent:") {
                current_user_agent = Some(line[11..].trim().to_string());
                in_target_section = current_user_agent.as_deref() == Some("*");
            } else if in_target_section && line.to_lowercase().starts_with("disallow:") {
                let path = line[9..].trim().to_string();
                if !path.is_empty() {
                    self.disallow.push(path);
                }
            }
        }

        Ok(())
    }

    pub fn start(&mut self) {
        self.get_scraping_policy().expect("Failed to get policy");

        for _ in 0..1000000 {
            let url = self.get_next_url();
            println!("{}", url);
            let delay = time::Duration::from_millis(self.config.delay as u64);
            thread::sleep(delay);

            let doc = match self.get_document(&url) {
                Ok(doc) => doc,
                Err(_) => String::from("Error in requests")
            };

            let _ = match self.update_metadata(&doc) {
                Ok(_) => (),
                Err(_) => continue
            };

            let _ = match self.update_document(&doc, &url) {
                Ok(_) => (),
                Err(_) => continue
            };
        }
    }
}
