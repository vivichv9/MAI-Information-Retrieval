use crate::config;

pub struct Crawler<'a> {
    config:  &'a config::Config,
}

impl<'a> Crawler<'a> {
    pub fn new(config: &'a config::Config) -> Self {
        Self { config }
    }

    pub fn get_document(self) {
        unimplemented!()
    }

    pub fn update_metadata(self) {
        unimplemented!()
    }

    pub fn update_document(self) {
        unimplemented!()
    }
}