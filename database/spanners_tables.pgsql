DROP TABLE IF EXISTS data CASCADE;
DROP TABLE IF EXISTS jobs CASCADE;
DROP TYPE IF EXISTS STATUS_TYPE;

CREATE TYPE STATUS_TYPE AS ENUM ('Waiting', 'Running', 'Success', 'Failed', 'Aborted');

-- Prototypical representation. Will be changed depending on userauth etc.
-- CREATE TABLE users(
--     user_id     SERIAL PRIMARY KEY  NOT NULL,
--     role        TEXT,
--     max_storage int 
-- );

-- Data can contain a request or a response message, stored in binary_data.
CREATE TABLE data(
    data_id     SERIAL PRIMARY KEY NOT NULL,
    binary_data BYTEA   NOT NULL
);

CREATE TABLE jobs(
    job_id      SERIAL PRIMARY KEY  NOT NULL,
    -- Prototypical, dummy user id that is not yet linked to a user table
    user_id         INT             NOT NULL,
    time_received   TIMESTAMP       DEFAULT now(),
    starting_time   TIMESTAMP,
    end_time        TIMESTAMP,
    ogdf_runtime    BIGINT,
    status          TEXT            NOT NULL DEFAULT 'waiting',
    stdout_msg      TEXT,
    error_msg       TEXT,
    request_id      INT,
    response_id     INT,
    CONSTRAINT fk_request
        FOREIGN KEY(request_id) 
        REFERENCES data(data_id)
        ON DELETE SET NULL,
    CONSTRAINT fk_response
        FOREIGN KEY(response_id) 
        REFERENCES data(data_id)
        ON DELETE SET NULL

);