#include "sd_card.h"
#include "SD_MMC.h"
#include <string.h>
#include "common.h"

#define TF_VFS_IS_NULL(RET)                           \
    if (NULL == tf_vfs)                               \
    {                                                 \
        log_i("[Sys SD Card] Mount Failed"); \
        return RET;                                   \
    }

int photo_file_num = 0;
char file_name_list[DIR_FILE_NUM][DIR_FILE_NAME_MAX_LEN];

static fs::FS *tf_vfs = NULL;

void release_file_info(File_Info *info)
{
    File_Info *cur_node = NULL; // 记录当前节点
    if (NULL == info)
    {
        return;
    }
    for (cur_node = info->next_node; NULL != cur_node;)
    {
        // 判断是不是循环一圈回来了
        if (info->next_node == cur_node)
        {
            break;
        }
        File_Info *tmp = cur_node; // 保存准备删除的节点
        cur_node = cur_node->next_node;
        free(tmp);
    }
    free(info);
}

void join_path(char *dst_path, const char *pre_path, const char *rear_path)
{
    while (*pre_path != 0)
    {
        *dst_path = *pre_path;
        ++dst_path;
        ++pre_path;
    }
    if (*(pre_path - 1) != '/')
    {
        *dst_path = '/';
        ++dst_path;
    }

    if (*rear_path == '/')
    {
        ++rear_path;
    }
    while (*rear_path != 0)
    {
        *dst_path = *rear_path;
        ++dst_path;
        ++rear_path;
    }
    *dst_path = 0;
}

/*
 * get file basename
 */
static const char *get_file_basename(const char *path)
{
    // 获取最后一个'/'所在的下标
    const char *ret = path;
    for (const char *cur = path; *cur != 0; ++cur)
    {
        if (*cur == '/')
        {
            ret = cur + 1;
        }
    }
    return ret;
}


void SdCard::init()
{
    log_i("SdCard init");
#ifdef USE_SD_MMC
    //因为默认的40mHz频率在S3上有错误，会时不时卡住，搜索过后得到解决方案就是降低频率到20MHz
    SD_MMC.setPins(SDMC_CLK,SDMMC_CMD,SDMMC_D0);
    if(!SD_MMC.begin("/root", true,false,SDMMC_FREQ_DEFAULT))
    {
        log_i("SD Card Mount Failed");
        return;
    }
    tf_vfs = &SD_MMC;
    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE)
    {
        log_i("No SD Card attached");
        return;
    }

    log_i("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        log_i("MMC");
    }
    else if (cardType == CARD_SD)
    {
        log_i("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        log_i("SDHC");
    }
    else
    {
        log_i("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    log_i("SD Card Size: %lluMB\n", cardSize);
#else 
    ////为了不出错，单独使用一组SPI，和别的不复用
    SPIClass *sd_spi = new SPIClass(HSPI);          // another SPI
    sd_spi->begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS); // Replace default HSPI pins
    if (!SD.begin(SD_SS, *sd_spi, 80000000))        // SD-Card SS pin is 15

    // if (!SD.begin(SD_SS))        // 
    {
        log_i("Card Mount Failed");
        return;
    }
    tf_vfs = &SD;
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        log_i("No SD card attached");
        return;
    }

    log_i("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        log_i("MMC");
    }
    else if (cardType == CARD_SD)
    {
        log_i("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        log_i("SDHC");
    }
    else
    {
        log_i("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    log_i("SD Card Size: %lluMB\n", cardSize);

#endif

}

void SdCard::listDir(const char *dirname, uint8_t levels)
{
    TF_VFS_IS_NULL()

    log_i("Listing directory: %s\n", dirname);
    photo_file_num = 0;

    File root = tf_vfs->open(dirname);
    if (!root)
    {
        log_i("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        log_i("Not a directory");
        return;
    }

    int dir_len = strlen(dirname) + 1;

    File file = root.openNextFile();
    while (file && photo_file_num < DIR_FILE_NUM)
    {
        if (file.isDirectory())
        {
            log_i("  DIR : %s",file.name());
            if (levels)
            {
                listDir(file.name(), levels - 1);
            }
        }
        else
        {
            log_i("  FILE: ");
            // 只取文件名 保存到file_name_list中
            strncpy(file_name_list[photo_file_num], file.name() + dir_len, DIR_FILE_NAME_MAX_LEN - 1);
            file_name_list[photo_file_num][strlen(file_name_list[photo_file_num]) - 4] = 0;

            char file_name[FILENAME_MAX_LEN] = {0};
            sprintf(file_name, "%s/%s.bin", dirname, file_name_list[photo_file_num]);
            log_i("%s",file_name);
            ++photo_file_num;
            log_i("  SIZE: %d",file.size());
        }
        file = root.openNextFile();
    }
    log_i("%d",photo_file_num);
}

File_Info *SdCard::listDir(const char *dirname)
{
    TF_VFS_IS_NULL(NULL)

    log_i("Listing directory: %s\n", dirname);

    File root = tf_vfs->open(dirname);
    if (!root)
    {
        log_i("Failed to open directory");
        return NULL;
    }
    if (!root.isDirectory())
    {
        log_i("Not a directory");
        return NULL;
    }

    int dir_len = strlen(dirname) + 1;

    // 头节点的创建（头节点用来记录此文件夹）
    File_Info *head_file = (File_Info *)malloc(sizeof(File_Info));
    head_file->file_type = FILE_TYPE_FOLDER;
    head_file->file_name = (char *)malloc(dir_len);
    // 将文件夹名赋值给头节点（当作这个节点的文件名）
    strncpy(head_file->file_name, dirname, dir_len - 1);
    head_file->file_name[dir_len - 1] = 0;
    head_file->front_node = NULL;
    head_file->next_node = NULL;

    File_Info *file_node = head_file;

    File file = root.openNextFile();
    while (file)
    {
        // if (levels)
        // {
        //     listDir(file.name(), levels - 1);
        // }
        const char *fn = get_file_basename(file.name());
        // 字符数组长度为实际字符串长度+1
        int filename_len = strlen(fn);
        if (filename_len > FILENAME_MAX_LEN - 10)
        {
            log_i("Filename is too long.");
        }

        // 创建新节点
        file_node->next_node = (File_Info *)malloc(sizeof(File_Info));
        // 让下一个节点指向当前节点
        // （此时第一个节点的front_next会指向head节点，等遍历结束再调一下）
        file_node->next_node->front_node = file_node;
        // file_node指针移向节点
        file_node = file_node->next_node;

        // 船家创建新节点的文件名
        file_node->file_name = (char *)malloc(filename_len);
        strncpy(file_node->file_name, fn, filename_len); //
        file_node->file_name[filename_len] = 0;          //
        // 下一个节点赋空
        file_node->next_node = NULL;

        char tmp_file_name[FILENAME_MAX_LEN] = {0};
        // sprintf(tmp_file_name, "%s/%s", dirname, file_node->file_name);
        join_path(tmp_file_name, dirname, file_node->file_name);
        if (file.isDirectory())
        {
            file_node->file_type = FILE_TYPE_FOLDER;
            // 类型为文件夹
            log_i("  DIR : %s",tmp_file_name);
        }
        else
        {
            file_node->file_type = FILE_TYPE_FILE;
            // 类型为文件
            log_i("  FILE: %s",tmp_file_name);
            log_i("  SIZE: %d",file.size());
        }

        file = root.openNextFile();
    }

    if (NULL != head_file->next_node)
    {
        // 将最后一个节点的next_node指针指向 head_file->next_node
        file_node->next_node = head_file->next_node;
        // 调整第一个数据节点的front_node指针（非head节点）
        head_file->next_node->front_node = file_node;
    }
    return head_file;
}

void SdCard::createDir(const char *path)
{
    TF_VFS_IS_NULL()

    log_i("Creating Dir: %s\n", path);
    if (tf_vfs->mkdir(path))
    {
        log_i("Dir created");
    }
    else
    {
        log_i("mkdir failed");
    }
}

void SdCard::removeDir(const char *path)
{
    TF_VFS_IS_NULL()

    log_i("Removing Dir: %s\n", path);
    if (tf_vfs->rmdir(path))
    {
        log_i("Dir removed");
    }
    else
    {
        log_i("rmdir failed");
    }
}

void SdCard::readFile(const char *path)
{
    TF_VFS_IS_NULL()

    log_i("Reading file: %s\n", path);

    File file = tf_vfs->open(path);
    if (!file)
    {
        log_i("Failed to open file for reading");
        return;
    }

    log_i("Read from file: ");
    while (file.available())
    {
        // Serial.write(file.read());
        log_i("%d",file.read());
    }
    file.close();
}

String SdCard::readFileLine(const char *path, int num)
{
    TF_VFS_IS_NULL("")

    log_i("Reading file: %s line: %d\n", path, num);

    File file = tf_vfs->open(path);
    if (!file)
    {
        return ("Failed to open file for reading");
    }

    char *p = buf;
    while (file.available())
    {
        char c = file.read();
        if (c == '\n')
        {
            num--;
            if (num == 0)
            {
                *(p++) = '\0';
                String s(buf);
                s.trim();
                return s;
            }
        }
        else if (num == 1)
        {
            *(p++) = c;
        }
    }
    file.close();

    return String("error parameter!");
}

void SdCard::writeFile(const char *path, const char *info)
{
    TF_VFS_IS_NULL()

    log_i("Writing file: %s\n", path);

    File file = tf_vfs->open(path, FILE_WRITE);
    if (!file)
    {
        log_i("Failed to open file for writing");
        return;
    }
    if (file.println(info))
    {
        log_i("Write succ");
    }
    else
    {
        log_i("Write failed");
    }
    file.close();
}

File SdCard::open(const String &path, const char *mode)
{
    // TF_VFS_IS_NULL(RET)

    return tf_vfs->open(path, mode);
}

void SdCard::appendFile(const char *path, const char *message)
{
    TF_VFS_IS_NULL()

    log_i("Appending to file: %s\n", path);

    File file = tf_vfs->open(path, FILE_APPEND);
    if (!file)
    {
        log_i("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        log_i("Message appended");
    }
    else
    {
        log_i("Append failed");
    }
    file.close();
}

void SdCard::renameFile(const char *path1, const char *path2)
{
    TF_VFS_IS_NULL()

    log_i("Renaming file %s to %s\n", path1, path2);
    if (tf_vfs->rename(path1, path2))
    {
        log_i("File renamed");
    }
    else
    {
        log_i("Rename failed");
    }
}

boolean SdCard::deleteFile(const char *path)
{
    TF_VFS_IS_NULL(false)

    log_i("Deleting file: %s\n", path);
    if (tf_vfs->remove(path))
    {
        log_i("File deleted");
        return true;
    }
    else
    {
        log_i("Delete failed");
    }
    return false;
}

boolean SdCard::deleteFile(const String &path)
{
    TF_VFS_IS_NULL(false)

    log_i("Deleting file: %s\n", path);
    if (tf_vfs->remove(path))
    {
        log_i("File deleted");
        return true;
    }
    else
    {
        log_i("Delete failed");
    }
    return false;
}

void SdCard::readBinFromSd(const char *path, uint8_t *buf)
{
    TF_VFS_IS_NULL()

    File file = tf_vfs->open(path);
    size_t len = 0;
    if (file)
    {
        len = file.size();

        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }

        file.close();
    }
    else
    {
        log_i("Failed to open file for reading");
    }
}

void SdCard::writeBinToSd(const char *path, uint8_t *buf)
{
    TF_VFS_IS_NULL()

    File file = tf_vfs->open(path, FILE_WRITE);
    if (!file)
    {
        log_i("Failed to open file for writing");
        return;
    }

    size_t i;
    for (i = 0; i < 2048; i++)
    {
        file.write(buf, 512);
    }
    file.close();
}

void SdCard::fileIO(const char *path)
{
    TF_VFS_IS_NULL()

    File file = tf_vfs->open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file)
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        log_i("%u bytes read for %u ms\n", flen, end);
        file.close();
    }
    else
    {
        log_i("Failed to open file for reading");
    }

    file = tf_vfs->open(path, FILE_WRITE);
    if (!file)
    {
        log_i("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++)
    {
        file.write(buf, 512);
    }
    end = millis() - start;
    log_i("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}
