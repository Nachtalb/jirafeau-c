#include "jirafeau.h"
#include <stdio.h>
#include <string.h>

void show_help() {
  printf("jirafeau CLI\n");
  printf("Usage:\n");
  printf("  jirafeau <host> <command> [options]:\n");
  printf("\n");
  printf("Commands:\n");
  printf("  upload <file> [options]\n");
  printf("    -t, --time [time]\n");
  printf("    -o, --one-time-download\n");
  printf("    -k, --key [key]\n");
  printf("    -u, --upload-password [password]\n");
  printf("\n");
  printf("  download <file_id> [options]\n");
  printf("    -k, --key [key]\n");
  printf("    -c, --crypt-key [crypt-key]\n");
  printf("    -o, --output-file [output-file]\n");
  printf("\n");
  printf("  delete <file_id> [options]\n");
  printf("    -d, --delete-key [delete-key]\n");
  printf("\n");
}

void subcommand_upload(int argc, char *argv[]) {
  char *file_path         = argv[3];
  char *time              = NULL;
  char *upload_password   = NULL;
  char *key               = NULL;
  int   one_time_download = 0;

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

  UploadResult *result =
    jirafeau_upload(file_path, time, upload_password, one_time_download, key);

  if (result && result->file_id && result->delete_key) {
    printf("File ID: %s\n", result->file_id);
    printf("Delete Key: %s\n", result->delete_key);
    if (result->crypt_key) {
      printf("Crypt Key: %s\n", result->crypt_key);
    }
  } else {
    printf("Upload failed.\n");
  }
}

void subcommand_download(int argc, char *argv[]) {
  char *file_id     = argv[3];
  char *output_file = NULL;
  char *key         = NULL;
  char *crypt_key   = NULL;
  char *final_file;

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

  final_file = jirafeau_download(file_id, output_file, key, crypt_key);

  if (final_file) {
    printf("%s\n", final_file);
  }
}

void subcommand_delete(int argc, char *argv[]) {
  // Code for delete command
}

int main(int argc, char *argv[]) {
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
