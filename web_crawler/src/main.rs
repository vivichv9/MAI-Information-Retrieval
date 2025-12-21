pub mod config;
pub mod crawler;
pub mod models;
pub mod schema;

use anyhow::Result;
use dotenvy::dotenv;
use std::fs;
use yaml_rust::YamlLoader;

use crate::config::Config;
use crate::crawler::Crawler;


fn main() -> Result<()> {
    dotenv().ok();

    let config_path: String = std::env::args().nth(1).expect("no config file given");
    let yaml_config: String = fs::read_to_string(config_path)?;
    let crawler_cfg = YamlLoader::load_from_str(&yaml_config)?;

    let delay = crawler_cfg[0]["crawler"]["delay"]
        .as_i64()
        .expect("Delay is not configured");
    let domain = crawler_cfg[0]["crawler"]["domain"]
        .as_str()
        .expect("Domain is not configured");
    let initial_path = crawler_cfg[0]["crawler"]["initial_path"]
        .as_str()
        .expect("Initial path is not configured");
    let mongo_uri = crawler_cfg[0]["crawler"]["mongo"]
        .as_str()
        .expect("Mongo URI is not configured");
    let pg_uri = crawler_cfg[0]["crawler"]["postgres"]
        .as_str()
        .expect("Postrges URI is not configured");

    let crawler_cfg = Config::new(
        delay as u32,
        String::from(domain),
        String::from(initial_path),
        mongo_uri,
        pg_uri,
    );


    let mut crawler = Crawler::new(crawler_cfg);

    crawler.start();

    Ok(())
}
