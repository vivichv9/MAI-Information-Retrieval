pub mod models;
pub mod schema;
pub mod crawler;
pub mod config;

use diesel::prelude::*;
use dotenvy::dotenv;
use std::env;


use models::Urls;


pub fn establish_connection() -> PgConnection {
    dotenv().ok();

    let database_url = env::var("PG_URL").expect("PG_URL must be set");
    PgConnection::establish(&database_url)
        .unwrap_or_else(|_| panic!("Error connecting to {}", database_url))
}

fn main() -> Result<(), ()> {
    use self::schema::urls::dsl::*;

    let connection = &mut establish_connection();
    let results = urls
        .filter(is_parsed.eq(true))
        .limit(5)
        .select(Urls::as_select())
        .load(connection)
        .expect("Error loading urls");

    println!("Displaying {} posts", results.len());
    for url in results {
        println!("{}", url.path);
        println!("{}", url.is_parsed);
    }
    Ok(())
}
