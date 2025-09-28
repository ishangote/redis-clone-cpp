#include "storage/database.h"

#include <gtest/gtest.h>

TEST(DatabaseTest, BasicSetGet) {
    redis_clone::storage::Database db;
    db.set("foo", "bar");
    auto val = db.get("foo");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "bar");
    EXPECT_FALSE(db.get("nonexistent").has_value());
}

TEST(DatabaseTest, DeleteKey) {
    redis_clone::storage::Database db;
    db.set("key", "value");
    EXPECT_TRUE(db.del("key"));
    EXPECT_FALSE(db.get("key").has_value());
    EXPECT_FALSE(db.del("key"));  // deleting again returns false
}

TEST(DatabaseTest, ExistsKey) {
    redis_clone::storage::Database db;
    db.set("a", "b");
    EXPECT_TRUE(db.exists("a"));
    db.del("a");
    EXPECT_FALSE(db.exists("a"));
}
