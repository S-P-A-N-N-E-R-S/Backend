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
    -- Type of the request, eg 'generic' or some special request
    type        INT     NOT NULL,
    binary_data BYTEA   NOT NULL
);

CREATE TABLE jobs(
    job_id      SERIAL PRIMARY KEY  NOT NULL,
    job_name        TEXT            NOT NULL DEFAULT '',
    -- the handler type (for example 'dijkstra'). Might be empty if request was not generic
    handler_type    TEXT            NOT NULL DEFAULT '',
    user_id         INT             NOT NULL,
    -- Time the request was recived by the server
    time_received   TIMESTAMPTZ     DEFAULT now(),
    -- time a handler started a job working on this request
    starting_time   TIMESTAMPTZ,
    -- time the job of this request ended (regardless of success)
    end_time        TIMESTAMPTZ,
    -- pure runtime of the call to the ogdf algorithm
    ogdf_runtime    BIGINT          NOT NULL DEFAULT 0,
    -- status of the request at this moment
    status          TEXT            NOT NULL DEFAULT 'waiting',
    -- Everything the algorithm printed on stdout (if not terminated)
    stdout_msg      TEXT            NOT NULL DEFAULT '',
    -- Everything the algorithm printed on stderr (if not terminated) 
    error_msg       TEXT            NOT NULL DEFAULT '',
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
