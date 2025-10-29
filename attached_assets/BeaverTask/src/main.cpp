#include "../include/httplib.h"
#include "../include/json.hpp"
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

struct Task {
    std::string id;
    std::string title;
    std::string description;
    std::string tag;
    std::string dueDate;
    std::string startTime;
    std::string endTime;
    bool allDay;
    std::string location;
    int reminderMinutes;
    std::string color;
    bool completed;
};

std::vector<Task> tasks;
std::mutex tasks_mutex;
int task_id_counter = 1;

std::string generateId() {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    std::ostringstream oss;
    oss << "task_" << task_id_counter++;
    return oss.str();
}

void to_json(json& j, const Task& t) {
    j = json{
        {"id", t.id},
        {"title", t.title},
        {"description", t.description},
        {"tag", t.tag},
        {"dueDate", t.dueDate},
        {"startTime", t.startTime},
        {"endTime", t.endTime},
        {"allDay", t.allDay},
        {"location", t.location},
        {"reminderMinutes", t.reminderMinutes},
        {"color", t.color},
        {"completed", t.completed}
    };
}

void from_json(const json& j, Task& t) {
    j.at("title").get_to(t.title);
    t.description = j.value("description", "");
    t.tag = j.value("tag", "");
    t.dueDate = j.value("dueDate", "");
    t.startTime = j.value("startTime", "");
    t.endTime = j.value("endTime", "");
    t.allDay = j.value("allDay", false);
    t.location = j.value("location", "");
    t.reminderMinutes = j.value("reminderMinutes", 0);
    t.color = j.value("color", "");
    t.completed = j.value("completed", false);
}

std::string getContentType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".json") != std::string::npos) return "application/json";
    return "text/plain";
}

int main() {
    httplib::Server svr;

    svr.set_mount_point("/", "./public");

    svr.Get("/api/tasks", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        json j = tasks;
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/api/tasks", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            
            Task task;
            from_json(body, task);
            task.id = generateId();
            
            {
                std::lock_guard<std::mutex> lock(tasks_mutex);
                tasks.push_back(task);
            }
            
            json response;
            to_json(response, task);
            
            res.status = 201;
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid request body"})", "application/json");
        }
    });

    svr.Put(R"(/api/tasks/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string id = req.matches[1];
            auto body = json::parse(req.body);
            
            std::lock_guard<std::mutex> lock(tasks_mutex);
            
            for (auto& task : tasks) {
                if (task.id == id) {
                    if (body.contains("title")) task.title = body["title"];
                    if (body.contains("description")) task.description = body["description"];
                    if (body.contains("tag")) task.tag = body["tag"];
                    if (body.contains("dueDate")) task.dueDate = body["dueDate"];
                    if (body.contains("startTime")) task.startTime = body["startTime"];
                    if (body.contains("endTime")) task.endTime = body["endTime"];
                    if (body.contains("allDay")) task.allDay = body["allDay"];
                    if (body.contains("location")) task.location = body["location"];
                    if (body.contains("reminderMinutes")) task.reminderMinutes = body["reminderMinutes"];
                    if (body.contains("color")) task.color = body["color"];
                    if (body.contains("completed")) task.completed = body["completed"];
                    
                    json response;
                    to_json(response, task);
                    
                    res.set_content(response.dump(), "application/json");
                    return;
                }
            }
            
            res.status = 404;
            res.set_content(R"({"error":"Task not found"})", "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid request body"})", "application/json");
        }
    });

    svr.Delete(R"(/api/tasks/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        std::lock_guard<std::mutex> lock(tasks_mutex);
        
        auto it = std::find_if(tasks.begin(), tasks.end(),
            [&id](const Task& t) { return t.id == id; });
        
        if (it != tasks.end()) {
            tasks.erase(it);
            res.status = 204;
        } else {
            res.status = 404;
            res.set_content(R"({"error":"Task not found"})", "application/json");
        }
    });

    std::cout << "===========================================\n";
    std::cout << "  Beavertask C++ Server Starting\n";
    std::cout << "===========================================\n";
    std::cout << "  Server: http://0.0.0.0:5000\n";
    std::cout << "  Framework: cpp-httplib\n";
    std::cout << "  Backend: C++17\n";
    std::cout << "===========================================\n\n";
    
    svr.listen("0.0.0.0", 5000);
    
    return 0;
}
