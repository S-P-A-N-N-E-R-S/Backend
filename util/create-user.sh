#!/bin/bash
set -e

USER_NAME="$1"
RAW_SALT=$(head -c16 /dev/urandom)
ESCAPED_SALT=$(echo -n "$RAW_SALT" | xxd -p)
HASH=$(echo -n "$2" | argon2 "$RAW_SALT" -id -t 2 -m 16 -p 1 -l 32 -r)

psql -c "INSERT INTO users (user_name, pw_hash, salt, role) VALUES ('$USER_NAME', decode('$HASH', 'hex'), decode('$ESCAPED_SALT', 'hex'), 0);" spanner_db
