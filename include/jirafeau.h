#ifndef JIRAFAEU_H
#define JIRAFAEU_H

#include <stdlib.h>

/**
 * Struct to hold the result of an upload operation.
 */
typedef struct {
  char *file_id;
  char *delete_key;
  char *crypt_key;
} UploadResult;

/**
 * Sets the host URL for the Jirafeau server.
 *
 * @param host_url URL of the Jirafeau server
 */
void jirafeau_set_host(const char *host_url);

/**
 * Uploads a file to the Jirafeau server.
 *
 * @param file_path Path to the file to be uploaded
 * @param time Time until the upload expires (optional)
 * @param upload_password Password for the upload (optional)
 * @param one_time_download Flag to enable one-time download (optional)
 * @param key Key for authorized access (optional)
 * @return A struct containing the FILE_ID, DELETE_KEY, and CRYPT_KEY
 */
UploadResult *jirafeau_upload(const char *file_path, const char *time,
                              const char *upload_password,
                              int one_time_download, const char *key);

/**
 * Downloads a file from the Jirafeau server.
 *
 * @param file_id ID of the file to be downloaded
 * @param output_path Path where the downloaded file will be saved
 * @param file_key Key for authorized access (optional)
 * @param crypt_key Crypt key for encrypted files (optional)
 * @return The path to the newly created file
 */
char *jirafeau_download(const char *file_id, const char *output_path,
                        const char *file_key, const char *crypt_key);

/**
 * Deletes a file from the Jirafeau server.
 *
 * @param file_id ID of the file to be deleted
 * @param delete_key Delete key for the file
 */
void jirafeau_delete(const char *file_id, const char *delete_key);

#endif // JIRAFAEU_H
