# Jirafeau CLI

This project provides a command-line interface for interacting with a Jirafeau
file host.

## Features

- Upload files with optional password and expiry time
- Download files with optional password
- Delete files

## Dependencies

- libcurl

## Building

To build the project, follow these steps:

```sh
mkdir build
cd build
cmake ..
make
./jirafeau
```

## Usage

Upload a file:

```bash
./jirafeau upload <file_path> [expiry_time] [one_time] [password]
```

Download a file:

```bash
./jirafeau download <download_url> [password] [output]
```

Delete a file:

```bash
./jirafeau delete <delete_url>
```

## License

LGPL License
