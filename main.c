#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char* absolute_to_relative(const char* absolute_path);
char* get_directory(const char* file_path);
void add_directory(const char *name, FILE *fp);
void add_source(const char *name, FILE *fp);
int is_directory(const char *filename);
int is_c_file(const char *filename);
int is_h_file(const char *filename);

void process_include_directories(const char *current_dir, FILE *fp);
void process_c_file(const char *current_dir, FILE *fp);

void test();

const char *path;

int main(int argc, char **argv) {
//    test();
    if (argc < 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    path = argv[1];
    DIR *dir;
    struct dirent *entry;

    FILE *fp = fopen("CMakeListsTemplate.txt", "w");
    if (fp == NULL) {
        perror("Unable to open file for writing");
        return 1;
    }

    fprintf(fp, "include_directories(\n");

    process_include_directories(path, fp);

    fprintf(fp, ")\nadd_definitions(-DDEBUG -DUSE_STDPERIPH_DRIVER -DSTM32F10X_HD)\n\nfile(GLOB_RECURSE SOURCES\n");

    process_c_file(path, fp);

    fprintf(fp, ")\n");
    fclose(fp);

    printf("CMakeLists.txt generated successfully.\n");

    return 0;
}

void process_include_directories(const char *current_dir, FILE *fp) {
    static int processed_files_num = 0;
    static char *processed_files[2048];
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(current_dir)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "%s/%s", current_dir, entry->d_name);
            if (is_directory(buf)) {
                // Ignore "." and ".." directories
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    process_include_directories(buf, fp);
                }
            } else {
                if (!is_directory(buf) && is_h_file(entry->d_name)) {
                    char *relative_path = absolute_to_relative(buf);
//                    if (!strstr(buf, "/")) { // This is to avoid adding files with absolute path in the second iteration
                        for (int i = 0; i < processed_files_num; i++) {
                            if (strcmp(processed_files[i], relative_path) == 0) {
                                free(relative_path);
                                relative_path = NULL;
                                break;
                            }
                        }
                        if (relative_path != NULL) {
                            add_directory(buf, fp);
                            processed_files[processed_files_num++] = relative_path;
                        }
//                    }
                }
            }
        }
        closedir(dir);
    } else {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }
}

void process_c_file(const char *current_dir, FILE *fp) {
    static int processed_files_num = 0;
    static char *processed_files[2048];
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(current_dir)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "%s/%s", current_dir, entry->d_name);
            if (is_directory(buf)) {
                // Ignore "." and ".." directories
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    process_c_file(buf, fp);
                }
            } else {
                if (!is_directory(buf) && is_c_file(entry->d_name)) {
                    char *relative_path = absolute_to_relative(buf);
//                    if (!strstr(buf, "/")) { // This is to avoid adding files with absolute path in the second iteration
                        for (int i = 0; i < processed_files_num; i++) {
                            if (strcmp(processed_files[i], relative_path) == 0) {
                                free(relative_path);
                                relative_path = NULL;
                                break;
                            }
                        }
                        if (relative_path != NULL) {
                            add_source(buf, fp);
                            processed_files[processed_files_num++] = relative_path;
                        }
//                    }
                }
            }
        }
        closedir(dir);
    } else {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }
}

char* absolute_to_relative(const char* absolute_path) {
    // 计算路径的长度
    int current_dir_len = strlen(path);
    int absolute_path_len = strlen(absolute_path);

    // 定义相对路径的最大长度
    int max_relative_len = current_dir_len + absolute_path_len + 3; // 路径分隔符 + '\0'

    // 分配相对路径的内存
    char* relative_path = (char*)malloc(max_relative_len * sizeof(char));

    // 初始化相对路径为空字符串
    relative_path[0] = '\0';

    // 寻找两个路径的共同前缀部分
    int i = 0;
    while (path[i] == absolute_path[i] && path[i] != '\0') {
        i++;
    }

    // 添加相对路径的起始部分
//    strcat(relative_path, ".\\");

    // 添加相对路径的后半部分
    strcat(relative_path, &absolute_path[i]);

    // 将Windows路径分隔符替换为Unix风格的分隔符
    char* ptr = relative_path;
    while (*ptr != '\0') {
        if (*ptr == '\\') {
            *ptr = '/';
        }
        ptr++;
    }

    return relative_path;
}

char* get_directory(const char* file_path) {
    // 查找文件路径中最后一个路径分隔符的位置
    const char* last_slash = strrchr(file_path, '/');
    const char* last_backslash = strrchr(file_path, '\\');

    // 确定使用哪个路径分隔符
    const char* last_separator = last_slash > last_backslash ? last_slash : last_backslash;

    if (last_separator == NULL) {
        // 如果找不到路径分隔符，则文件在当前目录下
        return strdup("./");
    } else {
        // 分配内存并拷贝文件夹路径
        int dir_len = last_separator - file_path + 1;
        char* directory = (char*)malloc((dir_len + 1) * sizeof(char));
        strncpy(directory, file_path, dir_len);
        directory[dir_len] = '\0';
        return directory;
    }
}

void add_directory(const char *name, FILE *fp) {
    char *relative_path = absolute_to_relative(name);
    char *directory = get_directory(relative_path);
    fprintf(fp, "        %s\n", directory);
}

void add_source(const char *name, FILE *fp) {
    char *relative_path = absolute_to_relative(name);
    char *directory = get_directory(relative_path);
    fprintf(fp, "        \"%s/*.c\"\n", directory);
    fprintf(fp, "        \"%s/**/*.c\"\n", directory);
}

int is_directory(const char *filename) {
    struct stat statbuf;
    if (stat(filename, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

int is_c_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    return dot && !strcmp(dot, ".c");
}

int is_h_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    return dot && !strcmp(dot, ".h");
}
