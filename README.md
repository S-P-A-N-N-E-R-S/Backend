# S.P.A.N.N.E.R.S. Backend

## Requirements

#### Libraries
* Protobuf (>= 3.17)
* boost (>= 1.71)
* cmake (>= 3.16)
* pqxx (>= 7.5)
* asio (>= 1.18)
* OpenSSL dev library (>= 1.1.1l)

#### Cloning
Clone the repository with submodules:
```bash
git clone --recurse-submodules https://gitpgtcs.informatik.uni-osnabrueck.de/spanners/backend
```

#### Install pqxx library manually
Install a version >= 7.5.2; depending on operating system might need to install manually.
* [Download](https://github.com/jtv/libpqxx) on GitHub
```bash
git checkout 7.5.2  # or other version number
./configure --enable-shared
make
sudo make install
```

#### Install protobuf manually
Ubuntu systems might require manuall installation of protobuf. 
Refer to the [protobuf installation guide](https://github.com/protocolbuffers/protobuf/blob/master/src/README.md).

Otherwise installation via e.g. `apt-get`
```bash
sudo apt-get update
sudo apt-get install protobuf-compiler
```

## Setting up the Database
* Install postgres
* To create superuser, first open the postgresql shell:
```bash
sudo -u postgres psql
```

```sql
CREATE SUPERUSER spanner_user WITH PASSWORD pwd;
CREATE DATABASE spanner_db;
\q
```
* Alternatively, use the `createuser` [command](https://www.postgresql.org/docs/current/app-createuser.html).

####
* Open a shell as user postgres and access the database:

```bash
sudo -su postgres
psql spanner_db
```
* On Arch-systems (and possibly other systems) you might need to initialize postgres and start the postgresql service:
```bash
initdb -D /var/lib/postgres/data
```

####
* To insert tables copy the code from [spanners_tables/pgsql](https://gitpgtcs.informatik.uni-osnabrueck.de/spanners/backend/-/tree/feat/scheduler-backend/database)
   * You can also use the command `psql spanner_db < database/spanners_tables.pgsql` if you're in the root of the backend repo
* `\dt` shows the tables in  the database

## Compile the Backend
### Server with TLS encryption
Compiling with TLS enabled requires a signed certificate and a key file in PEM encoding:
```bash
mkdir build
cd build
cmake ..
make -j8  # or however many cores you have
```
Run the application with two command line arguments:
```./apps/server <certificate-path> <key-path>```


### Server without TLS encryption:
The server can be build without TLS encryption e.g. for local use
Running the server application without TLS does not require any command line arguments
```bash
mkdir build
cd build
cmake -DUNENCRYPTED_CONNECTION=ON ..
make -j8  # or however many cores you have
```

## Run the Backend
```bash
./path/to/backend/apps/server
```

# Docker
The backend server can easily be run in a docker container without the need to configure a database on
your system.

## Installation
The following steps show how to install and run the backend Docker:
- Install docker on your system, see https://docs.docker.com/get-docker/
- If you want to run docker as non root user (optional), see https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
- Install Docker Compose (already included in Docker Desktop), see https://docs.docker.com/compose/install/
- Clone this repository recursively: `git clone --recursive <repository>`
- Run postgres container in the root directory: `docker-compose up postgres`
- Run backend database file in a separate terminal to remove old and create new tables:
  `docker-compose exec postgres psql spanner_db spanner_user -f /opt/backend/database/spanners_tables.pgsql`
- Stop postgres container.
- Start Docker: `docker-compose up`
- Done! The server is now running and accessible under `localhost:4711` on the host system.

## Notes
- Code changes can be made directly in the backend directory.
- Changes will be compiled when `docker-compose up` is called.
- Run commands in backend by `docker-compose exec backend <command>`. Replace `backend` with `postgres` to run commands
  on the postgres database docker.
- While `docker-compose up` is running, the shell can be accessed in other terminal by `docker-compose exec backend bash`
- The backend code is located on `/opt/backend`.
- Rebuild Docker images (helpful if changes have been made in the Dockerfile): `docker-compose up --build`
- In `docker-compose.yml` the TLS encryption is disabled as TLS requires a signed certificate and a key file. This allows
  quick and easy installation of the server. The encryption is highly recommended if you use compose in production.
  
## Licence
This backend is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
