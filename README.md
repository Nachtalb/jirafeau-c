# Jirafeau CLI & C API

This project provides a command-line interface and C api for interacting with a
[Jirafeau](https://gitlab.com/mojo42/Jirafeau) file host.

## Features

- 🚀 Upload files including optional password and expiry time
- 💻 Download files
- ❌ Delete files
- 🔒 Hosts with encryption enabled

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

> ⚠ Due to the fact that Jirafeau doesn't report errors with proper HTTP status
> codes and neither has a great api interface (and the fact I couldn't be
> bothered to parse HTML), it doesn't really return meaningful error messages.

### API

Until I write an actual example you can take a look at
[`main.c`](https://github.com/Nachtalb/jirafeau-c/blob/master/src/main.c) which
implements the CLI.

### CLI

```sh
# Upload
> ./jirafeau https://your.host.tdl upload test.png
File ID:    1beI2PNG
Delete Key: 47e9d

# Download
> ./jirafeau https://your.host.tdl download 1beI2PNG -o ../
../test.png

# Delete
> ./jirafeau https://your.host.tdl delete 1beI2PNG 47e9d
File deleted
```

```txt
Usage:
  jirafeau <host> <command> [options]:

Commands:
  upload <file> [options]
    -t, --time [minute|hour|day|week|fornight|(month)]
    -o, --one-time-download
    -k, --key [key]
    -u, --upload-password [password]

  download <file_id> [options]
    -k, --key [key]
    -c, --crypt-key [crypt-key]
    -o, --output-file [output-file]

  delete <file_id> <delete_key>
```

## License

LGPL License
