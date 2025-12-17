use diesel::prelude::{Connection, PgConnection};
use mongodb::{
    bson::Document,
    sync::{Client, Collection},
};

pub struct Config {
    delay: u32,
    domain: String,
    initial_path: String,
    collection: Collection<Document>,
    metadata_db: PgConnection,
}

impl Config {
    pub fn new(delay: u32, domain: String, initial_path: String, mongo_uri: &str, pg_uri: &str) -> Self {
        let mongo_client = Client::with_uri_str(mongo_uri).unwrap_or_else(|_| {
            panic!("Failed to initialize mongo client with uri: {}", mongo_uri)
        });

        let mongo_db = mongo_client.database("documents");
        let documents_collection: Collection<Document> = mongo_db.collection("movies");

        let pg_conn = PgConnection::establish(&pg_uri)
            .unwrap_or_else(|_| panic!("Error connecting to {}", pg_uri));

        Self {
            delay,
            domain,
            initial_path,
            collection: documents_collection,
            metadata_db: pg_conn,
        }
    }
}
