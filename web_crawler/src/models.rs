use diesel::prelude::*;

#[derive(Queryable, Selectable, Clone, Insertable)]
#[diesel(table_name = crate::schema::urls)]
#[diesel(check_for_backend(diesel::pg::Pg))]
pub struct Urls {
    pub path: String,
    pub is_parsed: bool,
}