#include "jirafeau.h"
#include <curl/curl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char * host_url;
static char * output_dir       = NULL;
static char * output_file_path = NULL;
static FILE * output_file      = NULL;
static Status state;

struct MemoryStruct {
  char * memory;
  size_t size;
};

static size_t write_request_body_to_memory(void *contents, size_t size,
                                           size_t nmemb, void *userdata) {
  size_t realsize          = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userdata;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    perror("Error: not enough memory\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static size_t handle_request_body(void *ptr, size_t size, size_t nmemb,
                                  void *stream) {
  if (strstr(ptr, "file is not found")) {
    state = FILE_NOT_FOUND;
    return 0;
  }

  if (!output_file) {
    state = ERROR;

    if (strstr(ptr, "File has been deleted")) {
      state = SUCCESS;
      return 0;
    }
    return 1;
  } else {
    state = SUCCESS;
    size_t written = fwrite(ptr, size, nmemb, output_file);
    return written;
  }
}

static void
set_output_file_by_content_disposition(const char *disposition_header) {
  const char *query    = "filename=\"";
  char *      start    = strstr(disposition_header, query);
  char *      filename = NULL;
  char *      output_path;

  if (start) {
    start += strlen(query);
    char *end = strstr(start, "\"");

    if (end) {
      size_t len = end - start;

      filename = (char *)malloc(len + 1);
      if (filename) {
        memcpy(filename, start, len);
        filename[len] = '\0';

        size_t out_dir_len = strlen(output_dir);
        output_file_path = (char *)malloc(out_dir_len + len + 2);
        if (output_file_path) {
          strcpy(output_file_path, output_dir);
          strcat(output_file_path, "/");
          strcat(output_file_path, filename);

          output_file = fopen(output_file_path, "wb");
          if (!output_file) {
            perror("Failed to open file\n");
          }
        }

        free(filename);
      }
    }
  }
}

static size_t handle_request_headers(char *buffer, size_t size, size_t nitems,
                                     void *userdata) {
  if (strncmp(buffer, "content-disposition:", 19) == 0) {
    set_output_file_by_content_disposition(buffer);
  }
  return nitems * size;
}

static void set_output_dir_or_file(const char *output_path) {
  if (output_path) {
    char *      temp_path = strdup(output_path);
    struct stat st        = { 0 };

    if (stat(temp_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        output_dir = strdup(temp_path);
        int len = strlen(output_dir);
        if (output_dir[len - 1] == '/') {
          output_dir[len - 1] = '\0';
        }
        free(temp_path);
        return;
      }
    }

    char *dir = dirname(temp_path);

    if (stat(dir, &st) == -1) {
      perror("Error: Provided output directory does not exist\n");
      free(temp_path);
      return;
    }

    if (S_ISDIR(st.st_mode) && strcmp(dir, output_path) == 0) {
      output_dir = strdup(dir);
      int len = strlen(output_dir);
      if (output_dir[len - 1] == '/') {
        output_dir[len - 1] = '\0';
      }
    } else {
      output_file_path = strdup(output_path);
      output_file      = fopen(output_path, "wb");

      if (!output_file) {
        perror("Error: Could not open output file\n");
        free(temp_path);
        return;
      }
    }

    free(temp_path);
  } else {
    output_dir = strdup(".");
  }
}

void jirafeau_set_host(const char *new_host_url) {
  if (new_host_url) {
    host_url = strdup(new_host_url);
  }
}

UploadResultT jirafeau_upload(const char *file_path, const char *time,
                              const char *upload_password,
                              int one_time_download, const char *key) {
  struct UploadResult result = { 0 };

  if (!host_url) {
    perror("`host_url` has not been defined previously to calling "
           "jirafeau_upload\n");
    result.state = ERROR;
    return result;
  }

  CURL *         curl;
  CURLcode       res;
  curl_mime *    mime;
  curl_mimepart *part;
  const char *   endpoint = "/script.php";

  struct MemoryStruct chunk;
  chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
  chunk.size   = 0;         /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char *url = malloc(strlen(host_url) + strlen(endpoint) + 1);
    snprintf(url, strlen(host_url) + strlen(endpoint) + 1, "%s%s", host_url,
             endpoint);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_request_body_to_memory);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    mime = curl_mime_init(curl);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, file_path);

    if (time) {
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "time");
      curl_mime_data(part, time, CURL_ZERO_TERMINATED);
    }

    if (upload_password) {
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "upload_password");
      curl_mime_data(part, upload_password, CURL_ZERO_TERMINATED);
    }

    if (one_time_download) {
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "one_time_download");
      curl_mime_data(part, one_time_download ? "1" : "0", CURL_ZERO_TERMINATED);
    }

    if (key) {
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "key");
      curl_mime_data(part, key, CURL_ZERO_TERMINATED);
    }

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      result.state = ERROR;
    } else {
      char *line = strtok(chunk.memory, "\n");
      if (!line) {
        result.state = ERROR;
        return result;
      }
      result.file_id = strdup(line);
      line           = strtok(NULL, "\n");
      if (!line) {
        fprintf(stderr, "%s\n", result.file_id);
        result.state = ERROR;
        return result;
      }
      result.delete_key = strdup(line);
      line = strtok(NULL, "\n");
      if (line) {
        result.crypt_key = strdup(line);
      }

      result.state = SUCCESS;
    }

    curl_easy_cleanup(curl);
    curl_mime_free(mime);
  }
  curl_global_cleanup();
  return result;
}

DownloadResultT jirafeau_download(const char *file_id, const char *output_path,
                                  const char *file_key, const char *crypt_key) {
  struct DownloadResult result = { 0 };

  set_output_dir_or_file(output_path);

  if (!output_dir && !output_file_path) {
    result.state = ERROR;
    return result;
  }

  char *endpoint_template = "%s/f.php?h=%s&d=1";
  int   len = strlen(host_url) + strlen(endpoint_template) - 4 + strlen(file_id) +
              +1; // -4 due to 2 %s in template and + 1 for \0
  char *url = malloc(len);
  snprintf(url, len, endpoint_template, host_url, file_id);

  CURL *   curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    if (crypt_key) {
      strcat(url, "&k=");
      strcat(url, crypt_key);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (!output_file) {
      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handle_request_headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_request_body);

    if (file_key) {
      char post_fields[256];
      snprintf(post_fields, sizeof(post_fields), "key=%s", file_key);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
    }

    res = curl_easy_perform(curl);

    if (state == SUCCESS && res != CURLE_OK) {
      state = ERROR;
    }

    result.state = state;
    if (result.state == SUCCESS) {
      result.download_path = strdup(output_file_path);
      fclose(output_file);
      free(output_file_path);
    } else if (output_file) {
      fclose(output_file);
      unlink(output_file_path);
    }

    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  return result;
}

DeleteResultT jirafeau_delete(const char *file_id, const char *delete_key) {
  struct DeleteResult result = { 0 };
  CURL *         curl;
  CURLcode       res;
  curl_mime *    mime;
  curl_mimepart *part;

  char *endpoint_template = "%s/f.php?h=%s&d=%s";
  int   len = strlen(host_url) + strlen(endpoint_template) - 6 + strlen(file_id) +
              strlen(delete_key) + 1; // -6 due to 3 %s in template and + 1 for \0
  char *url = malloc(len);
  snprintf(url, len, endpoint_template, host_url, file_id, delete_key);

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handle_request_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_request_body);

    mime = curl_mime_init(curl);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "do_delete");
    curl_mime_data(part, "1", CURL_ZERO_TERMINATED);

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    result.state = state;
  }

  curl_global_cleanup();
  return result;
}
