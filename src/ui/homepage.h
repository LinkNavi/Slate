#pragma once
#include <functional>
#include <string>

struct HomepageCallbacks {
    std::function<void()>                    on_new_project;
    std::function<void()>                    on_open_folder;
    std::function<void(const std::string&)>  on_open_recent;
    std::function<void()>                    on_open_settings;
    std::function<void(const std::string&)>  on_clone_repo;
};

struct RecentProject {
    std::string name;
    std::string path;
    std::string last_opened; // display string e.g. "2 days ago"
};

struct Homepage {
    HomepageCallbacks callbacks;

    void render(); // call each frame while no project is open

    // Populate from disk / config
    void set_recent(std::vector<RecentProject> projects);

private:
    void render_titlebar();
    void render_actions();
    void render_recent();
    void render_clone_bar();

    std::vector<RecentProject> m_recent;
    char m_clone_buf[512] = {};
    char m_search_buf[128] = {};
};
