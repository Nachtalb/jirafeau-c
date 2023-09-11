#include "jirafeau.h"
#include <curl/curl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *host_url         = "http://default_server.com";
static char *output_dir       = NULL;
static char *output_file_path = NULL;
static FILE *output_file      = NULL;

struct MemoryStruct {
  char * memory;
  size_t size;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb,
                                    void *userdata) {
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

void jirafeau_set_host(const char *new_host_url) {
  if (new_host_url) {
    host_url = strdup(new_host_url);
  }
}

static void open_file_to_write(const char *disposition_header) {
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
            perror("Failed to open file");
          }
        }

        free(filename);
      }
    }
  }
}

static size_t header_callback(char *buffer, size_t size, size_t nitems,
                              void *userdata) {
  if (strncmp(buffer, "content-disposition:", 19) == 0) {
    open_file_to_write(buffer);
  }
  return nitems * size;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  if (output_file) {
    size_t written = fwrite(ptr, size, nmemb, output_file);
    return written;
  }
  perror("Error: Still no output file defined when starting download. There "
         "may be something wrong with the request. Set output file manually "
         "for now.");
  return 0; // Return 0 to signal an errorb
}

UploadResult *jirafeau_upload(const char *file_path, const char *time,
                              const char *upload_password,
                              int one_time_download, const char *key) {
  CURL *         curl;
  CURLcode       res;
  curl_mime *    mime;
  curl_mimepart *part;

  struct MemoryStruct chunk;
  chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
  chunk.size   = 0;         /* no data at this point */

  UploadResult *result = (UploadResult *)calloc(1, sizeof(UploadResult));

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char *url = strdup(host_url);
    strcat(url, "/script.php");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

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
      fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
    } else {
      char *line = strtok(chunk.memory, "\n");
      if (!line) {
        perror("Response has no body");
        return NULL;
      }
      result->file_id    = strdup(line);
      line               = strtok(NULL, "\n");
      result->delete_key = strdup(line);
      line               = strtok(NULL, "\n");
      result->crypt_key  = strdup(line);
    }

    curl_easy_cleanup(curl);
    curl_mime_free(mime);
  }
  curl_global_cleanup();
  return result;
}

static void set_output_dir_or_file(const char *output_path) {
  if (output_path) {
    char *temp_path = strdup(output_path);
    char *dir       = dirname(temp_path);

    struct stat st = { 0 };

    if (stat(dir, &st) == -1) {
      perror("Error: Provided output directory does not exist");
      free(temp_path);
      return;
    }

    if (S_ISDIR(st.st_mode) && strcmp(dir, output_path) == 0) {
      output_dir = strdup(dir);
    } else {
      output_file_path = strdup(output_path);
      output_file      = fopen(output_path, "wb");

      if (!output_file) {
        perror("Error: Could not open output file");
        free(temp_path);
        return;
      }
    }

    free(temp_path);
  } else {
    output_dir = strdup(".");
  }
}

char *jirafeau_download(const char *file_id, const char *output_path,
                        const char *file_key, const char *crypt_key) {
  set_output_dir_or_file(output_path);

  char *   result = NULL;
  CURL *   curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char url[512];
    snprintf(url, sizeof(url), "%s/f.php?d=1&h=%s", host_url, file_id);

    if (crypt_key) {
      strcat(url, "&k=");
      strcat(url, crypt_key);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (output_file) {
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);
    } else {
      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    }

    if (file_key) {
      char post_fields[256];
      snprintf(post_fields, sizeof(post_fields), "key=%s", file_key);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
    }

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "Download error: %s\n", curl_easy_strerror(res));
    } else {
      result = strdup(output_file_path);
    }

    fclose(output_file);
    free(output_file_path);

    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  return result;
}
