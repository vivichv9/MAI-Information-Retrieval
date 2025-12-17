// @generated automatically by Diesel CLI.

diesel::table! {
    urls (path) {
        path -> Varchar,
        is_parsed -> Bool,
    }
}
