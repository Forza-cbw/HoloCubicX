#include <Arduino.h>
#include "FS.h"
// #include <LittleFS.h>
#include <SPIFFS.h>
#include <time.h>
#include "flash_fs.h"




/* You only need to format SPIFFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

#define FORMAT_LITTLEFS_IF_FAILED true

FlashFS::FlashFS()
{
    // SPIFFS初始化需要在setup启动后，如果在全局变量里初始化会报错
    // if (!SPIFFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    // {
    //     log_i("SPIFFS Mount Failed");
    //     return;
    // }

    // listDir("/", 0);
    // createDir("/mydir");
    // writeFile("/mydir/hello2.txt", "Hello2");
    // // writeFile("/mydir/newdir2/newdir3/hello3.txt", "Hello3");
    // writeFile2("/mydir/newdir2/newdir3/hello3.txt", "Hello3");
    // listDir("/", 3);
    // deleteFile("/mydir/hello2.txt");
    // // deleteFile("/mydir/newdir2/newdir3/hello3.txt");
    // deleteFile2("/mydir/newdir2/newdir3/hello3.txt");
    // removeDir("/mydir");
    // listDir("/", 3);
    // writeFile("/hello.txt", "Hello ");
    // appendFile("/hello.txt", "World!\r\n");
    // readFile("/hello.txt");
    // renameFile("/hello.txt", "/foo.txt");
    // readFile("/foo.txt");
    // deleteFile("/foo.txt");
    // testFileIO("/test.txt");
    // deleteFile("/test.txt");
}

FlashFS::~FlashFS()
{
}

void FlashFS::listDir(const char *dirname, uint8_t levels)
{
    log_i("Listing directory: %s\r\n", dirname);

    File root = SPIFFS.open(dirname);
    if (!root)
    {
        log_i("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        log_i(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            log_i("  DIR : %s",file.name());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            log_i("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

            if (levels)
            {
                listDir(file.name(), levels - 1);
            }
        }
        else
        {
            log_i("  FILE: %s,SIZE: %d",file.name(),file.size());

            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            log_i("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}

// void FlashFS::createDir(const char *path)
// {
//     // SPIFFS不支持目录
//     log_i("Creating Dir: %s\n", path);
//     if (SPIFFS.mkdir(path))
//     {
//         log_i("Dir created");
//     }
//     else
//     {
//         log_i("mkdir failed");
//     }
// }

// void FlashFS::removeDir(const char *path)
// {
//     log_i("Removing Dir: %s\n", path);
//     if (SPIFFS.rmdir(path))
//     {
//         log_i("Dir removed");
//     }
//     else
//     {
//         log_i("rmdir failed");
//     }
// }

uint16_t FlashFS::readFile(const char *path, uint8_t *info)
{
    log_i("Reading file: %s\r\n", path);

    File file = SPIFFS.open(path);
    uint16_t ret_len = 0;
    if (!file || file.isDirectory())
    {
        log_i("- failed to open file for reading");
        return ret_len;
    }

    // log_i("- read from file:");
    while (file.available())
    {
        ret_len += file.read(info + ret_len, 15);
        // Serial.write(file.read());
    }
    file.close();
    return ret_len;
}

void FlashFS::writeFile(const char *path, const char *message)
{
    log_i("Writing file: %s\r\n", path);

    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
    {
        log_i("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        log_i("- file written");
    }
    else
    {
        log_i("- write failed");
    }
    file.close();
}

void FlashFS::appendFile(const char *path, const char *message)
{
    log_i("Appending to file: %s\r\n", path);

    File file = SPIFFS.open(path, FILE_APPEND);
    if (!file)
    {
        log_i("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        log_i("- message appended");
    }
    else
    {
        log_i("- append failed");
    }
    file.close();
}

void FlashFS::renameFile(const char *src, const char *dst)
{
    log_i("Renaming file %s to %s\r\n", src, dst);
    if (SPIFFS.rename(src, dst))
    {
        log_i("- file renamed");
    }
    else
    {
        log_i("- rename failed");
    }
}

void FlashFS::deleteFile(const char *path)
{
    log_i("Deleting file: %s\r\n", path);
    if (SPIFFS.remove(path))
    {
        log_i("- file deleted");
    }
    else
    {
        log_i("- delete failed");
    }
}

// SPIFFS-like write and delete file

// See: https://github.com/esp8266/Arduino/blob/master/libraries/SPIFFS/src/SPIFFS.cpp#L60
// void FlashFS::writeFile2(const char *path, const char *message)
// {
//     if (!SPIFFS.exists(path))
//     {
//         if (strchr(path, '/'))
//         {
//             log_i("Create missing folders of: %s\r\n", path);
//             char *pathStr = strdup(path);
//             if (pathStr)
//             {
//                 char *ptr = strchr(pathStr, '/');
//                 while (ptr)
//                 {
//                     *ptr = 0;
//                     SPIFFS.mkdir(pathStr);
//                     *ptr = '/';
//                     ptr = strchr(ptr + 1, '/');
//                 }
//             }
//             free(pathStr);
//         }
//     }

//     log_i("Writing file to: %s\r\n", path);
//     File file = SPIFFS.open(path, FILE_WRITE);
//     if (!file)
//     {
//         log_i("- failed to open file for writing");
//         return;
//     }
//     if (file.print(message))
//     {
//         log_i("- file written");
//     }
//     else
//     {
//         log_i("- write failed");
//     }
//     file.close();
// }

// See:  https://github.com/esp8266/Arduino/blob/master/libraries/SPIFFS/src/SPIFFS.h#L149
// void FlashFS::deleteFile2(const char *path)
// {
//     log_i("Deleting file and empty folders on path: %s\r\n", path);

//     if (SPIFFS.remove(path))
//     {
//         log_i("- file deleted");
//     }
//     else
//     {
//         log_i("- delete failed");
//     }

//     char *pathStr = strdup(path);
//     if (pathStr)
//     {
//         char *ptr = strrchr(pathStr, '/');
//         if (ptr)
//         {
//             log_i("Removing all empty folders on path: %s\r\n", path);
//         }
//         while (ptr)
//         {
//             *ptr = 0;
//             SPIFFS.rmdir(pathStr);
//             ptr = strrchr(pathStr, '/');
//         }
//         free(pathStr);
//     }
// }

void FlashFS::testFileIO(const char *path)
{
    log_i("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
    {
        log_i("- failed to open file for writing");
        return;
    }

    size_t i;
    log_i("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            log_i(".");
        }
        file.write(buf, 512);
    }
    log_i("");
    uint32_t end = millis() - start;
    log_i(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = SPIFFS.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        log_i("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                log_i(".");
            }
            len -= toRead;
        }
        log_i("");
        end = millis() - start;
        log_i("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        log_i("- failed to open file for reading");
    }
}

bool analyseParam(char *info, int argc, char **argv)
{
    int cnt; // 记录解析到第几个参数
    for (cnt = 0; cnt < argc; ++cnt)
    {
        argv[cnt] = info;
        while (*info != '\n')
        {
            ++info;
        }
        *info = 0;
        ++info;
    }
    return true;
}