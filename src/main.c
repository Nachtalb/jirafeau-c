#include "jirafeau.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void show_help() {
  printf("jirafeau CLI\n");
  printf("Usage:\n");
  printf("  jirafeau <host> <command> [options]:\n");
  printf("\n");
  printf("Commands:\n");
  printf("  upload <file> [options]\n");
  printf("    -t, --time [minute|hour|day|week|fornight|(month)]\n");
  printf("    -o, --one-time-download\n");
  printf("    -k, --key [key]\n");
  printf("    -u, --upload-password [password]\n");
  printf("    -r, --randomised-name\n");
  printf("\n");
  printf("  download <file_id> [options]\n");
  printf("    -k, --key [key]\n");
  printf("    -c, --crypt-key [crypt-key]\n");
  printf("    -o, --output-file [output-file]\n");
  printf("\n");
  printf("  delete <file_id> <delete_key>\n");
  printf("\n");
}

static void random_string(char *str, size_t len) {
  const char charset[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i < len; ++i) {
    int index = rand() % (sizeof(charset) - 1);
    str[i] = charset[index];
  }
  str[len] = '\0';
}

void subcommand_upload(int argc, char *argv[]) {
  char *        file_path         = argv[3];
  char *        time              = "month";
  char *        upload_password   = NULL;
  char *        key               = NULL;
  int           one_time_download = 0;
  char *        filename;
  UploadResultT result;

  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--time") == 0) {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        time = argv[++i];
      } else {
        perror("Missing value for -t/--time\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-o") == 0 ||
               strcmp(argv[i], "--one-time-download") == 0) {
      one_time_download = 1;
    } else if (strcmp(argv[i], "-r") == 0 ||
               strcmp(argv[i], "--randomised-name") == 0) {
      char random_stem[10];
      random_string(random_stem, 9);
      char *extension = strrchr(file_path, '.');
      filename = (char *)malloc(strlen(extension) + 10 + 1);
      snprintf(filename, strlen(extension) + 10 + 1, "%s%s", random_stem,
               extension);
    } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--key") == 0) {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        key = argv[++i];
      } else {
        perror("Missing value for -k/--key\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-u") == 0 ||
               strcmp(argv[i], "--upload-password") == 0) {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        upload_password = argv[++i];
      } else {
        perror("Missing value for -u/--upload-password\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  if (filename) {
    result = jirafeau_upload(file_path, time, upload_password,
                             one_time_download, key, filename);
    free(filename);
  } else {
    jirafeau_upload(file_path, time, upload_password, one_time_download, key,
                    filename);
  }

  if (result.state == SUCCESS) {
    char *host_url = jirafeau_get_host();

    int   len      = strlen(host_url) + strlen(result.file_id) + 10;
    char *file_url = (char *)malloc(len);
    snprintf(file_url, len, "%s/f.php?h=%s", host_url, result.file_id);

    if (isatty(STDOUT_FILENO)) {
      printf("\033[1;32mURL\033[0m          %s\n", file_url);
      printf("\033[1;34mPreview URL\033[0m  %s&p=1\n", file_url);
      printf("\033[1;35mDownload URL\033[0m %s&d=1\n", file_url);
      printf("\033[1;31mDelete URL\033[0m   %s&d=%s\n", file_url,
             result.delete_key);
      printf("\n");
      printf("\033[1;36mFile ID\033[0m      %s\n", result.file_id);
      printf("\033[1;33mDelete Key\033[0m   %s\n", result.delete_key);
      if (result.crypt_key) {
        printf("\033[1;37mCrypt Key\033[0m    %s\n", result.crypt_key);
      }
    } else {
      printf("{\"host\": \"%s\", "
             "\"file_url\": \"%s\", "
             "\"file_preview_url\": \"%s&p=1\", "
             "\"file_download_url\": \"%s&d=1\", "
             "\"file_delete_url\": \"%s&d=%s\", "
             "\"file_id\":\"%s\",\"delete_key\":\"%s\",\"crypt_key\":\"%s\"}\n",
             host_url, file_url, file_url, file_url, file_url,
             result.delete_key, result.file_id, result.delete_key,
             result.crypt_key ? result.crypt_key : "");
    }

    free(file_url);
  } else {
    printf("Upload failed.\n");
    exit(EXIT_FAILURE);
  }
}

void subcommand_download(int argc, char *argv[]) {
  char *          file_id     = argv[3];
  char *          output_file = NULL;
  char *          key         = NULL;
  char *          crypt_key   = NULL;
  DownloadResultT result;

  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--key") == 0) {
      key = argv[++i];
    } else if (strcmp(argv[i], "-c") == 0 ||
               strcmp(argv[i], "--crypt-key") == 0) {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        crypt_key = argv[++i];
      } else {
        perror("Missing value for -c/--crypt-key\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-o") == 0 ||
               strcmp(argv[i], "--output-file") == 0) {
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        output_file = argv[++i];
      } else {
        perror("Missing value for -o/--output-file\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  result = jirafeau_download(file_id, output_file, key, crypt_key);

  switch (result.state) {
  case ERROR:
    perror("An error occurred\n");
    exit(EXIT_FAILURE);
    break;

  case FILE_NOT_FOUND:
    perror("File could not be found\n");
    exit(EXIT_FAILURE);
    break;

  case SUCCESS:
    printf("%s\n", result.download_path);
    break;
  }
}

void subcommand_delete(int argc, char *argv[]) {
  if (argc < 5) {
    perror("You have to define both the 'file_id' and the 'delete_key'");
    exit(EXIT_FAILURE);
  }
  char *        file_id    = argv[3];
  char *        delete_key = argv[4];
  DeleteResultT result;

  result = jirafeau_delete(file_id, delete_key);

  switch (result.state) {
  case ERROR:
    perror("An error occurred\n");
    exit(EXIT_FAILURE);
    break;

  case FILE_NOT_FOUND:
    perror("File could not be found, already deleted?\n");
    exit(EXIT_FAILURE);
    break;

  case SUCCESS:
    printf("File deleted\n");
    break;
  }
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      show_help();
      return 1;
    }
  }

  char *host    = argv[1];
  char *command = argv[2];

  jirafeau_set_host(host);

  if (strcmp(command, "upload") == 0) {
    subcommand_upload(argc, argv);
  } else if (strcmp(command, "download") == 0) {
    subcommand_download(argc, argv);
  } else if (strcmp(command, "delete") == 0) {
    subcommand_delete(argc, argv);
  } else {
    printf("Invalid command. Use --help for usage information.\n");
    return 1;
  }

  return 0;
}
