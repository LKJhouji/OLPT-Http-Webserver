#include <iostream>
#include <mysql/mysql.h>

int main() {
    MYSQL* mysql = mysql_init(mysql);
    if (mysql != NULL) {
        std::cerr << "Error: mysql_init() returned NULL." << std::endl;
        return 1;
    }
    return 0;
}