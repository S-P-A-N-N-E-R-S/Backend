# S.P.A.N.N.E.R.S. Backend

## Requirements

#### Libraries
* Protobuf (>= 3.17)
* boost (>= 1.71)
* cmake (>= 3.16)
* pqxx (>= 7.5)
* asio (>= 1.18)

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
```bash
mkdir build
cd build
cmake ..
make -j8  # or however many cores you have
```

## Run the Backend
```bash
./path/to/backend/apps/server
```
