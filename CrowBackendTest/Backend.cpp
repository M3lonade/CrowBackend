#include "crow.h"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <iostream>
#include "crow/middlewares/cors.h"

int main() {
    crow::App<crow::CORSHandler> app;

    // Set up CORS. This is necessary in our case, I lost my mind figuring out why I was getting 204 API error and it was because CORS wasn't enabled.
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")  // Allow all origins explicitly
        .methods("POST"_method, "OPTIONS"_method)  // Only POST and OPTIONS allowed
        .headers("Content-Type")  // Allow content type header
        .allow_credentials();

    // Route to handle registration
    CROW_ROUTE(app, "/register").methods("POST"_method)
        ([&](const crow::request& req) {
        try {
            auto x = crow::json::load(req.body);
            if (!x) {
                auto response = crow::response(400, "Bad Request");
                std::cout << "Response: " << response.code << " " << response.body << std::endl;
                return response;
            }

            std::string username = x["username"].s();
            std::string password = x["password"].s();

            sql::mysql::MySQL_Driver* driver;
            sql::Connection* con;
            sql::PreparedStatement* pstmt;

            driver = sql::mysql::get_mysql_driver_instance();
            con = driver->connect("tcp://127.0.0.1:3306", "root", "root"); //Login details for the mysql server, currently on a local server (IP, Username, Password)
            con->setSchema("crow_test");
            
            //Prepared statements so people don't try SQL injections :)
            pstmt = con->prepareStatement("INSERT INTO users(username, password) VALUES (?, ?)");
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->execute();

            delete pstmt;
            delete con;

            auto response = crow::response(200, "User registered successfully");
            std::cout << "Response: " << response.code << " " << response.body << std::endl;
            return response;
        }
        catch (sql::SQLException& e) {
            std::cerr << "SQL Error: " << e.what() << std::endl;
            auto response = crow::response(500, "Database Error: " + std::string(e.what()));
            std::cout << "Response: " << response.code << " " << response.body << std::endl;
            return response;
        }
        catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            auto response = crow::response(500, "Internal Server Error");
            std::cout << "Response: " << response.code << " " << response.body << std::endl;
            return response;
        }
            });

    app.port(8080).multithreaded().run();
}