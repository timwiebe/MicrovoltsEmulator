# 3.3 Setting up the database

Starting from version 2.0, I introduce MariaDB. This is much better since it allows you to have multiple regionals servers connect to the same, central database.
This section shows setup for both Windows and Linux.

## How to Set Up the MariaDB Database [WINDOWS]
Before running the servers, you’ll need a MariaDB database properly configured with the right tables and data. Here's how to do it step by step:

### Step 0: Install MariaDB
If you haven’t already installed MariaDB:

- Go to https://mariadb.org/download/
- Download the version that matches your OS
- During installation, make sure to remember the root password (you’ll need it shortly)

### Step 0.1 (Optional but recommended for an easy setup)
- Find `my.ini` inside your MariaDB folder. (Usually it can be found in `"C:\Program Files\MariaDB 11.6\data\my.ini"`
- Replace its contents with the following:
```cpp
[mysqld]
datadir="C:/Program Files/MariaDB 11.6/data"
port=3305
innodb_buffer_pool_size=1967M

[client]
port=3305
plugin-dir="C:/Program Files/MariaDB 11.6/lib/plugin"
```

Of course, make sure the paths are correct for you and that you have the necessary things installed (like innoDB).


### Step 1: Start mariaDB
Simply open a terminal and type `net start mariadb`. By default this will run on port 3306 (unless you did the optional step 0.1, in which case the port will be 3305), so make sure to use the correct database port on the `config.ini` file!

### Step 2: Create the Database
Once MariaDB is installed:

- Open the MariaDB command line or a GUI like HeidiSQL, DBeaver, or MySQL Workbench.

- Log in using your root credentials.

- Run this command to create the database: `CREATE DATABASE microvolts-db;`

### Step 3: Import the Tables
You’ve been provided with a file called `microvolts-db.`. This file contains the entire database structure and necessary insertions. You can find it in the root directory.

To import it:
#### Option A – Using Command Line
- `mysql -u root -p microvolts-db < path/to/microvolts-db.sql`
  
(You’ll be prompted to enter your root password)

#### Option B – Using GUI (e.g., HeidiSQL)
- Open your database in the GUI
- Open the SQL file `microvolts-db`
- Run the whole script
This will create all tables and insert initial data (like items, shop content, etc.)

### Step 4: Set the Password via Environment Variable
For security, the server expects your database password to be stored in an environment variable (not directly in the config).

For example, if in `setup.ini` you wrote: `PasswordEnvironmentName = MICRO_DB_PW`

On Windows:
- Press Win + S → Search for “Environment Variables”
- Click “Edit the system environment variables”
- Add a new System Variable:
  - Name: MICRO_DB_PW
  - Value: your_db_password_here
Done!


## How to Set Up the MariaDB Database [LINUX]
### Step 0: Install MariaDB
- Debian/Ubuntu: 
```cpp
sudo apt update
sudo apt install mariadb-server mariadb-client
```

- Fedora/CentOS/RHEL: `sudo dnf install mariadb-server mariadb`

Start and enable the service:
```cpp
sudo systemctl start mariadb
sudo systemctl enable mariadb
```

### Step 0.1 (Optional but recommended for an easy setup)
- Edit the MariaDB configuration file (usually found at `/etc/mysql/mariadb.conf.d/50-server.cnf or /etc/my.cnf`)
- Add or modify the following sections:
```cpp
[mysqld]
datadir=/var/lib/mysql
port=3305
innodb_buffer_pool_size=1967M

[client]
port=3305
```
Make sure paths correspond to your installation. `datadir` generally defaults to `/var/lib/mysql`


### Step 1: Start the service
`sudo systemctl start mariadb`
You can check if it's running with `sudo systemctl status mariadb`

### Step 2: Create the database
- `sudo mariadb -u root`
- `CREATE DATABASE microvolts-db;`
- `EXIT;`

### Step 3: Import the tables
`mysql -u root -p microvolts-db < /path/to/microvolts-db.sql` (you can find `microvolts-db` in this repository at the root directory)

### Step 4: Set password via environment variable
`export MICRO_DB_PW=your_db_password_here`

If you want it to be permanent:
- For bash: `echo 'export MICRO_DB_PW=your_db_password_here' >> ~/.bashrc`
- For zsh: `echo 'export MICRO_DB_PW=your_db_password_here' >> ~/.zshsrc` 


## Next
[3.4 Setting up the client](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/client_setup.md)
