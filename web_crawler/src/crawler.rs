use crate::config;

pub struct Crawler<'a> {
    config:  &'a config::Config,
}