use crate::config;
use crate::models::Urls;

use diesel::prelude::*;

pub struct Crawler {
    config: config::Config,
}

impl Crawler {
    pub fn new(config: config::Config) -> Self {
        Self { config }
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
            return String::from(&self.config.initial_path);
        }

        results.into_iter().next().unwrap().path
    }

    pub fn get_document(&mut self) {
        let url = self.get_next_url();
    }

    pub fn update_metadata(self) {
        unimplemented!()
    }

    pub fn update_document(self) {
        unimplemented!()
    }
}