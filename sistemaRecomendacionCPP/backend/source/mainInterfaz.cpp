#define CROW_MAIN
#define CROW_USE_BOOST

#include "crow.h"
#include <cctype>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

#include "../header/recommendationSystem.h"
#include "../header/timer.h"

static const std::string kRatingsCsvPath = "../out/user_ratings_custom.csv";

struct AuthUser
{
    int id;
    std::string username;
    std::string password;
};

static std::vector<AuthUser> g_auth_users;
static std::mutex g_auth_mutex;
static const std::string kAuthCsvPath = "../out/auth_users.csv";

static void ensureRatingsCsvExists()
{
    std::ifstream in(kRatingsCsvPath);
    if (in.good())
        return;
    std::ofstream out(kRatingsCsvPath);
    out << "userId,movieId,rating,timestamp\n";
}

static void ensureAuthCsvExists()
{
    std::ifstream in(kAuthCsvPath);
    if (in.good())
        return;
    std::ofstream out(kAuthCsvPath, std::ios::out);
    out << "id,username,password\n";
}

static void loadAuthUsersFromCsv()
{
    std::lock_guard<std::mutex> lock(g_auth_mutex);
    g_auth_users.clear();
    ensureAuthCsvExists();
    std::ifstream in(kAuthCsvPath);
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;
        std::stringstream ss(line);
        std::string idStr, username, password;
        if (!std::getline(ss, idStr, ','))
            continue;
        if (!std::getline(ss, username, ','))
            continue;
        if (!std::getline(ss, password, ','))
            continue;
        AuthUser u;
        u.id = std::stoi(idStr);
        u.username = username;
        u.password = password;
        g_auth_users.push_back(u);
    }
}

static int getNextAuthUserIdUnsafe()
{
    int maxId = 0;
    for (const auto &u : g_auth_users)
        if (u.id > maxId)
            maxId = u.id;
    return maxId + 1;
}

static bool usernameExistsUnsafe(const std::string &username)
{
    for (const auto &u : g_auth_users)
        if (u.username == username)
            return true;
    return false;
}

static bool appendAuthUserToCsv(const AuthUser &user)
{
    std::ofstream out(kAuthCsvPath, std::ios::app);
    if (!out.is_open())
        return false;
    out << user.id << "," << user.username << "," << user.password << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// BUG FIX #1: rewrite CSV on rating (avoid duplicates accumulating on disk)
// Previously appendRatingToCsv always appended, so on server restart the old
// overwritten in-memory rating was re-read and then overwritten again – causing
// the CSV to grow unboundedly with stale rows. Now we rewrite the whole file.
// ─────────────────────────────────────────────────────────────────────────────
static std::mutex g_ratings_mutex;

// In-memory store that mirrors the CSV so we can rewrite cleanly
static std::vector<std::tuple<int, int, float, long>> g_custom_ratings;

static void persistRatingsToCsv()
{
    std::ofstream out(kRatingsCsvPath, std::ios::out | std::ios::trunc);
    out << "userId,movieId,rating,timestamp\n";
    for (auto &[uid, mid, r, ts] : g_custom_ratings)
        out << uid << "," << mid << "," << r << "," << ts << "\n";
}

static void upsertRatingInMemoryAndCsv(int userId, int movieId, float rating)
{
    std::lock_guard<std::mutex> lock(g_ratings_mutex);
    long timestamp = std::time(nullptr);
    bool found = false;
    for (auto &[uid, mid, r, ts] : g_custom_ratings)
    {
        if (uid == userId && mid == movieId)
        {
            r = rating;
            ts = timestamp;
            found = true;
            break;
        }
    }
    if (!found)
        g_custom_ratings.emplace_back(userId, movieId, rating, timestamp);
    persistRatingsToCsv();
}

static void loadCustomRatings(RecommendationSystem &sistema)
{
    ensureRatingsCsvExists();
    std::ifstream in(kRatingsCsvPath);
    std::string line;
    std::getline(in, line); // skip header

    std::lock_guard<std::mutex> lock(g_ratings_mutex);
    g_custom_ratings.clear();

    while (std::getline(in, line))
    {
        if (line.empty())
            continue;
        std::stringstream ss(line);
        std::string u, m, r, t;
        std::getline(ss, u, ',');
        std::getline(ss, m, ',');
        std::getline(ss, r, ',');
        std::getline(ss, t, ',');

        int uid = std::stoi(u);
        int mid = std::stoi(m);
        float rtg = std::stof(r);
        long ts = t.empty() ? 0L : std::stol(t);

        // Upsert into in-memory store
        bool found = false;
        for (auto &[eu, em, er, ets] : g_custom_ratings)
        {
            if (eu == uid && em == mid)
            {
                er = rtg;
                ets = ts;
                found = true;
                break;
            }
        }
        if (!found)
            g_custom_ratings.emplace_back(uid, mid, rtg, ts);

        // ─────────────────────────────────────────────────────────────────
        // BUG FIX #2: addRatingAndUser already handles new userId insertion,
        // so we do NOT need a movies.find() guard here. Custom ratings from
        // real users reference valid movieIds already in the dataset.
        // ─────────────────────────────────────────────────────────────────
        sistema.addRatingAndUser(uid, mid, rtg, t);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Sync dataset users → g_auth_users IN MEMORY ONLY.
//
// The ratings.csv dataset has millions of users that were never in auth_users.csv.
// We create AuthUser entries for all of them so the login/recommendation system
// can find them, but we do NOT write them to the CSV (that would be gigabytes).
// They get auto-credentials:  username = "user_<id>",  password = "1234"
//
// Users that already exist in g_auth_users (manually created) are skipped so
// their custom username/password is preserved.
// ─────────────────────────────────────────────────────────────────────────────
// Crea una cuenta en auth_users.csv para cada userId que el RecommendationSystem
// ya tiene en memoria (cargado desde ratings.csv en su constructor).
// NO relee el CSV — usa sistema.getUsers() que ya esta en RAM.
// Credenciales por defecto: username = user_<id>, password = 1234
// Usuarios manuales existentes en g_auth_users se respetan (se saltan por id).
static void syncDatasetUsersToAuth(const RecommendationSystem &sistema)
{
    // 1. Lookup rapido de ids que ya tienen cuenta (manuales o de sync previo)
    std::unordered_set<int> existingIds;
    existingIds.reserve(g_auth_users.size());
    for (const auto &u : g_auth_users)
        existingIds.insert(u.id);

    // 2. Abrir CSV de auth en append — solo escribimos los nuevos
    ensureAuthCsvExists();
    std::ofstream authOut(kAuthCsvPath, std::ios::app);
    if (!authOut.is_open())
    {
        std::cerr << "[AUTH SYNC] ERROR: No se pudo abrir " << kAuthCsvPath << "\n";
        return;
    }

    // 3. Iterar sobre sistema.getUsers() — ya esta todo en RAM, sin releer disco
    const auto &datasetUsers = sistema.getUsers();
    std::cout << "[AUTH SYNC] Usuarios en sistema (dataset): " << datasetUsers.size() << "\n";
    std::cout << "[AUTH SYNC] Usuarios ya en auth CSV:       " << existingIds.size() << "\n";

    int created = 0;
    for (const int uid : datasetUsers)
    {
        if (existingIds.count(uid))
            continue; // ya tiene cuenta, saltar

        AuthUser u;
        u.id = uid;
        u.username = "user_" + std::to_string(uid);
        u.password = "1234";

        authOut << u.id << "," << u.username << "," << u.password << "\n";
        g_auth_users.push_back(u);
        existingIds.insert(uid);
        ++created;
    }

    std::cout << "[AUTH SYNC] Cuentas nuevas creadas: " << created << "\n";
    std::cout << "[AUTH SYNC] Total en auth ahora:    " << g_auth_users.size() << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// When a dataset user logs in for the first time with user_<id>/1234 and then
// wants to change their username/password, they call /api/auth/update.
// We persist only THAT change to the CSV (append), not the whole set.
// ─────────────────────────────────────────────────────────────────────────────
static bool findAuthUserByIdUnsafe(int id, AuthUser &found)
{
    for (auto &u : g_auth_users)
        if (u.id == id)
        {
            found = u;
            return true;
        }
    return false;
}

static bool findAuthUserByCredentialsUnsafe(const std::string &username,
                                            const std::string &password,
                                            AuthUser &found)
{
    for (const auto &u : g_auth_users)
    {
        if (u.username == username && u.password == password)
        {
            found = u;
            return true;
        }
    }
    return false;
}

// ─── CORS middleware ──────────────────────────────────────────────────────────
struct CORS
{
    struct context
    {
    };

    void before_handle(crow::request &req, crow::response &res, context &)
    {
        // Obtener el origen de la petición (en lugar de usar "*")
        std::string origin = req.get_header_value("Origin");

        if (!origin.empty())
        {
            // Responder con el mismo origen específico
            res.set_header("Access-Control-Allow-Origin", origin);
            res.set_header("Access-Control-Allow-Credentials", "true");
        }
        else
        {
            // Fallback para peticiones sin Origin
            res.set_header("Access-Control-Allow-Origin", "*");
        }

        // Permitir TODOS los métodos HTTP que necesitas
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");

        // 🔥 PERMITIR MÚLTIPLES CABECERAS (este era tu error principal)
        res.set_header("Access-Control-Allow-Headers",
                       "Content-Type, Authorization, X-Requested-With, Accept, Origin");

        // Opcional: cachear la respuesta preflight por 1 hora
        res.set_header("Access-Control-Max-Age", "3600");

        // Si es una petición OPTIONS (pre-flight), responder inmediatamente
        if (req.method == crow::HTTPMethod::OPTIONS)
        {
            res.code = 204; // No Content
            res.end();
            return;
        }
    }

    void after_handle(crow::request &req, crow::response &res, context &)
    {
        // También en after_handle, asegurar que las cabeceras estén presentes
        std::string origin = req.get_header_value("Origin");

        if (!origin.empty())
        {
            res.set_header("Access-Control-Allow-Origin", origin);
            res.set_header("Access-Control-Allow-Credentials", "true");
        }
        else
        {
            res.set_header("Access-Control-Allow-Origin", "*");
        }

        // Asegurar que las cabeceras estén para la respuesta final
        if (res.get_header_value("Access-Control-Allow-Methods").empty())
        {
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        }

        if (res.get_header_value("Access-Control-Allow-Headers").empty())
        {
            res.set_header("Access-Control-Allow-Headers",
                           "Content-Type, Authorization, X-Requested-With");
        }
    }
};
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    crow::App<CORS> app;

    loadAuthUsersFromCsv();

    RecommendationSystem sistema;

    // 1. Sync manual auth users -> recommendation system
    for (const auto &u : g_auth_users)
        if (!sistema.userExists(u.id))
            sistema.addUserWithId(u.id);

    loadCustomRatings(sistema);

    // 2. Sync dataset users -> auth (CSV + memory)
    //    sistema.getUsers() ya tiene TODOS los userId del ratings.csv (cargados
    //    en el constructor). Creamos user_<id>/1234 para los que no tengan cuenta.
    //    Persiste en auth_users.csv (append, solo los nuevos).
    {
        std::lock_guard<std::mutex> lock(g_auth_mutex);
        syncDatasetUsersToAuth(sistema);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // AUTH: REGISTER
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/auth/register").methods("POST"_method)([&sistema](const crow::request &req)
                                                                 {
        auto body = crow::json::load(req.body);
        std::string username, password;
        if (body) { username = body["username"].s(); password = body["password"].s(); }
        if (username.empty() || password.empty())
            return crow::response(400, R"({"error":"Faltan campos"})");

        std::lock_guard<std::mutex> lock(g_auth_mutex);
        if (usernameExistsUnsafe(username))
            return crow::response(409, R"({"error":"Usuario ya existe"})");

        AuthUser newUser;
        newUser.id       = getNextAuthUserIdUnsafe();
        newUser.username = username;
        newUser.password = password;

        if (!appendAuthUserToCsv(newUser))
            return crow::response(500, R"({"error":"Error guardando"})");

        g_auth_users.push_back(newUser);
        sistema.addUserWithId(newUser.id);

        crow::json::wvalue resp;
        resp["id"]       = newUser.id;
        resp["username"] = username;
        return crow::response(201, resp); });

    // ══════════════════════════════════════════════════════════════════════════
    // AUTH: LOGIN
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)([](const crow::request &req)
                                                              {
        auto body = crow::json::load(req.body);
        std::string username, password;
        if (body) { username = body["username"].s(); password = body["password"].s(); }
        if (username.empty() || password.empty())
            return crow::response(400, R"({"error":"Faltan campos"})");

        std::lock_guard<std::mutex> lock(g_auth_mutex);
        AuthUser foundUser;
        if (!findAuthUserByCredentialsUnsafe(username, password, foundUser))
            return crow::response(401, R"({"error":"Credenciales invalidas"})");

        crow::json::wvalue resp;
        resp["id"]       = foundUser.id;
        resp["username"] = foundUser.username;
        return crow::response(200, resp); });

    // ══════════════════════════════════════════════════════════════════════════
    // AUTH: UPDATE CREDENTIALS
    // Allows dataset users (auto-created as user_<id>/1234) to set their own
    // username and password. Only persists to CSV if not already there.
    // POST /api/auth/update  { id, new_username, new_password, current_password }
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/auth/update").methods("POST"_method)([](const crow::request &req)
                                                               {
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, R"({"error":"Body invalido"})");

        int         id           = body["id"].i();
        std::string newUsername  = body["new_username"].s();
        std::string newPassword  = body["new_password"].s();
        std::string currentPass  = body["current_password"].s();

        if (newUsername.empty() || newPassword.empty() || currentPass.empty())
            return crow::response(400, R"({"error":"Faltan campos"})");

        std::lock_guard<std::mutex> lock(g_auth_mutex);

        // Verify current password
        AuthUser target;
        if (!findAuthUserByIdUnsafe(id, target))
            return crow::response(404, R"({"error":"Usuario no existe"})");
        if (target.password != currentPass)
            return crow::response(401, R"({"error":"Password actual incorrecto"})");

        // Check new username not taken by someone else
        for (const auto &u : g_auth_users)
            if (u.username == newUsername && u.id != id)
                return crow::response(409, R"({"error":"Username ya en uso"})");

        // Update in-memory
        for (auto &u : g_auth_users)
        {
            if (u.id == id)
            {
                u.username = newUsername;
                u.password = newPassword;
                break;
            }
        }

        // Persist to CSV (overwrite the whole file to keep it clean)
        // We only persist users that are NOT auto-generated (or have been updated),
        // detected by username not matching the pattern "user_<id>" with pass "1234".
        // Simpler: just persist everyone whose credentials differ from the default.
        {
            std::ofstream out(kAuthCsvPath, std::ios::out | std::ios::trunc);
            out << "id,username,password\n";
            for (const auto &u : g_auth_users)
            {
                std::string defaultUser = "user_" + std::to_string(u.id);
                bool isDefault = (u.username == defaultUser && u.password == "1234");
                if (!isDefault)
                    out << u.id << "," << u.username << "," << u.password << "\n";
            }
        }

        crow::json::wvalue resp;
        resp["ok"]       = true;
        resp["id"]       = id;
        resp["username"] = newUsername;
        return crow::response(200, resp); });

    // ══════════════════════════════════════════════════════════════════════════
    // AUTH: LIST DATASET USERS (paginated, for admin/debug)
    // GET /api/auth/users?page=1&limit=50
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/auth/users").methods("GET"_method)([](const crow::request &req)
                                                             {
        auto page_p  = req.url_params.get("page");
        auto limit_p = req.url_params.get("limit");
        int page  = page_p  ? std::max(1, std::stoi(page_p))  : 1;
        int limit = limit_p ? std::min(200, std::stoi(limit_p)) : 50;

        std::lock_guard<std::mutex> lock(g_auth_mutex);
        int total = (int)g_auth_users.size();
        int start = (page - 1) * limit;
        int end   = std::min(start + limit, total);

        crow::json::wvalue::list arr;
        for (int i = start; i < end; ++i)
        {
            crow::json::wvalue item;
            item["id"]       = g_auth_users[i].id;
            item["username"] = g_auth_users[i].username;
            // Never expose password
            std::string def = "user_" + std::to_string(g_auth_users[i].id);
            item["is_default"] = (g_auth_users[i].username == def && g_auth_users[i].password == "1234");
            arr.push_back(std::move(item));
        }
        crow::json::wvalue res;
        res["total"] = total;
        res["page"]  = page;
        res["pages"] = (total + limit - 1) / limit;
        res["users"] = std::move(arr);
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // MOVIES: List with pagination & optional genre filter
    // GET /api/movies?page=1&limit=20&genre=Action
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/movies").methods("GET"_method)([&sistema](const crow::request &req)
                                                         {
        auto page_p  = req.url_params.get("page");
        auto limit_p = req.url_params.get("limit");
        auto genre_p = req.url_params.get("genre");

        int page  = page_p  ? std::max(1, std::stoi(page_p))  : 1;
        int limit = limit_p ? std::min(100, std::stoi(limit_p)): 20;
        std::string genre_filter = genre_p ? std::string(genre_p) : "";

        const auto &moviesMap = sistema.getMovies();
        const auto &links     = sistema.getLinks();

        std::vector<std::pair<int, std::pair<std::string, std::vector<std::string>>>> moviesList(
            moviesMap.begin(), moviesMap.end());

        // Filter by genre if requested
        if (!genre_filter.empty())
        {
            moviesList.erase(std::remove_if(moviesList.begin(), moviesList.end(),
                [&](const auto &m) {
                    const auto &genres = m.second.second;
                    return std::find(genres.begin(), genres.end(), genre_filter) == genres.end();
                }), moviesList.end());
        }

        int total = (int)moviesList.size();
        int start = (page - 1) * limit;
        int end   = std::min(start + limit, total);

        crow::json::wvalue res;
        res["total"] = total;
        res["page"]  = page;
        res["limit"] = limit;
        res["pages"] = (total + limit - 1) / limit;

        crow::json::wvalue::list arr;
        for (int i = start; i < end && i < total; ++i)
        {
            auto &[mid, info] = moviesList[i];
            crow::json::wvalue m;
            m["movie_id"] = mid;
            m["title"]    = info.first;

            int tmdbId = -1;
            auto it = links.find(mid);
            if (it != links.end()) tmdbId = it->second;
            m["tmdb_id"] = tmdbId;

            crow::json::wvalue::list genres;
            for (auto &g : info.second) genres.push_back(g);
            m["genres"] = std::move(genres);

            arr.push_back(std::move(m));
        }
        res["movies"] = std::move(arr);
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // MOVIES: Single movie detail
    // GET /api/movies/:id
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/movies/<int>").methods("GET"_method)([&sistema](int movie_id)
                                                               {
        const auto &moviesMap = sistema.getMovies();
        auto it = moviesMap.find(movie_id);
        if (it == moviesMap.end())
            return crow::response(404, R"({"error":"Pelicula no encontrada"})");

        const auto &links = sistema.getLinks();
        int tmdbId = -1;
        auto lit = links.find(movie_id);
        if (lit != links.end()) tmdbId = lit->second;

        crow::json::wvalue m;
        m["movie_id"] = movie_id;
        m["title"]    = it->second.first;
        m["tmdb_id"]  = tmdbId;

        crow::json::wvalue::list genres;
        for (auto &g : it->second.second) genres.push_back(g);
        m["genres"] = std::move(genres);

        return crow::response(200, m); });

    // ══════════════════════════════════════════════════════════════════════════
    // MOVIES: Search by title
    // GET /api/movies/search?q=toy&limit=10
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/movies/search").methods("GET"_method)([&sistema](const crow::request &req)
                                                                {
        auto q_p     = req.url_params.get("q");
        auto limit_p = req.url_params.get("limit");
        if (!q_p) return crow::response(400, R"({"error":"Falta parametro q"})");

        std::string query = q_p;
        int limit = limit_p ? std::min(50, std::stoi(limit_p)) : 10;

        // Lowercase query for case-insensitive search
        std::string lq = query;
        std::transform(lq.begin(), lq.end(), lq.begin(), ::tolower);

        const auto &moviesMap = sistema.getMovies();
        const auto &links     = sistema.getLinks();

        crow::json::wvalue::list arr;
        int count = 0;
        for (const auto &[mid, info] : moviesMap)
        {
            std::string lt = info.first;
            std::transform(lt.begin(), lt.end(), lt.begin(), ::tolower);
            if (lt.find(lq) != std::string::npos)
            {
                crow::json::wvalue m;
                m["movie_id"] = mid;
                m["title"]    = info.first;
                int tmdbId = -1;
                auto lit = links.find(mid);
                if (lit != links.end()) tmdbId = lit->second;
                m["tmdb_id"]  = tmdbId;
                crow::json::wvalue::list genres;
                for (auto &g : info.second) genres.push_back(g);
                m["genres"] = std::move(genres);
                arr.push_back(std::move(m));
                if (++count >= limit) break;
            }
        }
        crow::json::wvalue res;
        res["results"] = std::move(arr);
        res["count"]   = count;
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // GENRES: List all genres
    // GET /api/genres
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/genres").methods("GET"_method)([&sistema](const crow::request &)
                                                         {
        const auto &gm = sistema.getGenresMap();
        // Sort by frequency descending
        std::vector<std::pair<std::string,int>> glist(gm.begin(), gm.end());
        std::sort(glist.begin(), glist.end(), [](auto &a, auto &b){ return a.second > b.second; });

        crow::json::wvalue::list arr;
        for (auto &[g, cnt] : glist)
        {
            crow::json::wvalue item;
            item["genre"] = g;
            item["count"] = cnt;
            arr.push_back(std::move(item));
        }
        crow::json::wvalue res;
        res["genres"] = std::move(arr);
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // RATINGS: Rate or re-rate a movie
    // POST /api/calificar  { user_id, movie_id, rating }
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/calificar").methods("POST"_method)([&sistema](const crow::request &req)
                                                             {
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, R"({"error":"Body invalido"})");

        int   user_id  = body["user_id"].i();
        int   movie_id = body["movie_id"].i();
        float rating   = (float)body["rating"].d();

        if (rating < 0.5f || rating > 5.0f)
            return crow::response(400, R"({"error":"Rating debe estar entre 0.5 y 5.0"})");

        if (!sistema.userExists(user_id))
            return crow::response(404, R"({"error":"Usuario no existe"})");

        // ─────────────────────────────────────────────────────────────────
        // BUG FIX #3: recalificarPelicula returns silently if movieId not
        // found in movies map. We call addRatingAndUser directly so custom
        // users can rate ANY movie from the dataset without being blocked.
        // ─────────────────────────────────────────────────────────────────
        sistema.addRatingAndUser(user_id, movie_id, rating, "");

        // Persist (upsert – no duplicates)
        upsertRatingInMemoryAndCsv(user_id, movie_id, rating);

        crow::json::wvalue resp;
        resp["ok"]       = true;
        resp["user_id"]  = user_id;
        resp["movie_id"] = movie_id;
        resp["rating"]   = rating;
        return crow::response(200, resp); });

    // ══════════════════════════════════════════════════════════════════════════
    // RATINGS: Get ratings for a user
    // GET /api/ratings?user_id=1
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/ratings").methods("GET"_method)([&sistema](const crow::request &req)
                                                          {
        auto uid_p = req.url_params.get("user_id");
        if (!uid_p)
            return crow::response(400, R"({"error":"Falta user_id"})");

        int user_id = std::stoi(uid_p);
        if (!sistema.userExists(user_id))
            return crow::response(404, R"({"error":"Usuario no existe"})");

        const auto &umr   = sistema.getUserMovieRatings();
        const auto &movies = sistema.getMovies();
        const auto &links  = sistema.getLinks();

        crow::json::wvalue::list arr;
        auto it = umr.find(user_id);
        if (it != umr.end())
        {
            for (auto &[mid, rtg] : it->second)
            {
                crow::json::wvalue item;
                item["movie_id"] = mid;
                item["rating"]   = rtg;
                auto mit = movies.find(mid);
                if (mit != movies.end())
                {
                    item["title"] = mit->second.first;
                    int tmdbId = -1;
                    auto lit = links.find(mid);
                    if (lit != links.end()) tmdbId = lit->second;
                    item["tmdb_id"] = tmdbId;
                }
                arr.push_back(std::move(item));
            }
        }
        crow::json::wvalue res;
        res["user_id"] = user_id;
        res["ratings"] = std::move(arr);
        res["total"]   = (int)arr.size();  // note: arr moved, but size captured above
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // RATINGS: Delete a rating
    // DELETE /api/ratings  { user_id, movie_id }
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/ratings/delete").methods("POST"_method)([&sistema](const crow::request &req)
                                                                  {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, R"({"error":"Body invalido"})");
        int user_id  = body["user_id"].i();
        int movie_id = body["movie_id"].i();

        if (!sistema.userExists(user_id))
            return crow::response(404, R"({"error":"Usuario no existe"})");

        {
            std::lock_guard<std::mutex> lock(g_ratings_mutex);
            g_custom_ratings.erase(
                std::remove_if(g_custom_ratings.begin(), g_custom_ratings.end(),
                    [&](auto &t){ return std::get<0>(t)==user_id && std::get<1>(t)==movie_id; }),
                g_custom_ratings.end());
            persistRatingsToCsv();
        }
        return crow::response(200, R"({"ok":true})"); });

    // ══════════════════════════════════════════════════════════════════════════
    // RECOMMENDATIONS: Personalized
    // GET /api/recomendar?user_id=1&n=10&metric=cosine
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/recomendar").methods("GET"_method)([&sistema](const crow::request &req)
                                                             {
    auto user_param   = req.url_params.get("user_id");
    auto metric_param = req.url_params.get("metric");
    auto page_p       = req.url_params.get("page");
    auto limit_p      = req.url_params.get("limit");
    auto n_param      = req.url_params.get("n"); // vecinos KNN

    if (!user_param || !metric_param)
        return crow::response(400, R"({"error":"Faltan parametros"})");

    int user = std::stoi(user_param);
    std::string metric = metric_param;

    int page  = page_p  ? std::max(1, std::stoi(page_p))  : 1;
    int limit = limit_p ? std::min(100, std::stoi(limit_p)) : 20;
    int n     = n_param ? std::stoi(n_param) : 20;

    if (!sistema.userExists(user))
        return crow::response(404, R"({"error":"Usuario no existe"})");

    const auto &moviesMap = sistema.getMovies();
    const auto &links     = sistema.getLinks();

    // 1. Generar recomendaciones completas
    auto vecinos = sistema.knnParalelo(n, user, metric);
    auto recs    = sistema.recomendar(vecinos, user);
    auto final   = sistema.recomendarMovie(recs, vecinos, user, metric);

    // 2. Fallback si no hay personalizadas
    //if (final.empty())
      //  final = sistema.getPopularMovies(100); // más grande para paginar bien

    // 3. ORDEN GLOBAL por score DESC (clave para paginación correcta)
    std::sort(final.begin(), final.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first > b.first;
              });

    // 4. PAGINACIÓN
    int total = (int)final.size();
    int start = (page - 1) * limit;

    if (start >= total)
        start = total;

    int end = std::min(start + limit, total);

    crow::json::wvalue::list arr;

    for (int i = start; i < end; ++i)
    {
        auto &[score, mid] = final[i];

        crow::json::wvalue item;
        item["movie_id"] = mid;
        item["score"]    = score;

        auto mit = moviesMap.find(mid);
        if (mit != moviesMap.end())
        {
            item["title"] = mit->second.first;

            crow::json::wvalue::list genres;
            for (auto &g : mit->second.second)
                genres.push_back(g);

            item["genres"] = std::move(genres);
        }

        int tmdbId = -1;
        auto lit = links.find(mid);
        if (lit != links.end())
            tmdbId = lit->second;

        item["tmdb_id"] = tmdbId;

        arr.push_back(std::move(item));
    }

    // 5. RESPONSE con metadata
    crow::json::wvalue res;
    res["user_id"]     = user;
    res["metric"]      = metric;
    res["neighbors"]   = (int)vecinos.size();
    res["is_fallback"] = vecinos.empty();

    res["total"] = total;
    res["page"]  = page;
    res["limit"] = limit;
    res["pages"] = (total + limit - 1) / limit;

    res["movies"] = std::move(arr);

    return crow::response(200, res); });
    // ══════════════════════════════════════════════════════════════════════════
    // POPULAR: Top rated/most rated movies
    // GET /api/popular?limit=20
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/popular").methods("GET"_method)([&sistema](const crow::request &req)
                                                          {
        auto limit_p = req.url_params.get("limit");
        int limit = limit_p ? std::min(50, std::stoi(limit_p)) : 20;

        const auto &moviesMap = sistema.getMovies();
        const auto &links     = sistema.getLinks();

        auto popular = sistema.getPopularMovies(limit);

        crow::json::wvalue::list arr;
        for (auto &[score, mid] : popular)
        {
            crow::json::wvalue item;
            item["movie_id"] = mid;
            item["score"]    = score;
            auto mit = moviesMap.find(mid);
            if (mit != moviesMap.end())
            {
                item["title"] = mit->second.first;
                crow::json::wvalue::list genres;
                for (auto &g : mit->second.second) genres.push_back(g);
                item["genres"] = std::move(genres);
            }
            int tmdbId = -1;
            auto lit = links.find(mid);
            if (lit != links.end()) tmdbId = lit->second;
            item["tmdb_id"] = tmdbId;
            arr.push_back(std::move(item));
        }
        crow::json::wvalue res;
        res["movies"] = std::move(arr);
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // UNRATED: Movies by genre not yet rated by user
    // GET /api/unrated?user_id=1&genre=Action&limit=20
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/unrated").methods("GET"_method)([&sistema](const crow::request &req)
                                                          {
        auto uid_p   = req.url_params.get("user_id");
        auto genre_p = req.url_params.get("genre");
        auto limit_p = req.url_params.get("limit");

        if (!uid_p || !genre_p)
            return crow::response(400, R"({"error":"Faltan parametros"})");

        int user_id = std::stoi(uid_p);
        std::string genre = genre_p;
        int limit = limit_p ? std::stoi(limit_p) : 20;

        if (!sistema.userExists(user_id))
            return crow::response(404, R"({"error":"Usuario no existe"})");

        const auto &links = sistema.getLinks();
        auto result = sistema.getUnratedMoviesByGenre(user_id, genre, limit);

        crow::json::wvalue::list arr;
        for (auto &[mid, title, genres] : result)
        {
            crow::json::wvalue item;
            item["movie_id"] = mid;
            item["title"]    = title;
            int tmdbId = -1;
            auto lit = links.find(mid);
            if (lit != links.end()) tmdbId = lit->second;
            item["tmdb_id"]  = tmdbId;
            crow::json::wvalue::list gl;
            for (auto &g : genres) gl.push_back(g);
            item["genres"]   = std::move(gl);
            arr.push_back(std::move(item));
        }
        crow::json::wvalue res;
        res["movies"] = std::move(arr);
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // STATS: User stats (how many ratings, etc.)
    // GET /api/stats?user_id=1
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/stats").methods("GET"_method)([&sistema](const crow::request &req)
                                                        {
        auto uid_p = req.url_params.get("user_id");
        if (!uid_p)
            return crow::response(400, R"({"error":"Falta user_id"})");

        int user_id = std::stoi(uid_p);
        if (!sistema.userExists(user_id))
            return crow::response(404, R"({"error":"Usuario no existe"})");

        int rated = sistema.getNumberOfRatedMovies(user_id);

        const auto &umr = sistema.getUserMovieRatings();
        float avg = 0.0f;
        auto it = umr.find(user_id);
        if (it != umr.end() && !it->second.empty())
        {
            float sum = 0;
            for (auto &[mid, r] : it->second) sum += r;
            avg = sum / (float)it->second.size();
        }

        crow::json::wvalue res;
        res["user_id"]      = user_id;
        res["rated_movies"] = rated;
        res["avg_rating"]   = avg;
        res["total_users"]  = (int)sistema.getUsers().size();
        res["total_movies"] = (int)sistema.getMovies().size();
        return crow::response(200, res); });

    // ══════════════════════════════════════════════════════════════════════════
    // HEALTH CHECK
    // GET /api/health
    // ══════════════════════════════════════════════════════════════════════════
    CROW_ROUTE(app, "/api/health").methods("GET"_method)([&sistema](const crow::request &)
                                                         {
        crow::json::wvalue res;
        res["status"]       = "ok";
        res["users"]        = (int)sistema.getUsers().size();
        res["movies"]       = (int)sistema.getMovies().size();
        return crow::response(200, res); });

    std::cout << "Servidor corriendo en http://localhost:8080\n";
    app.port(8080).multithreaded().run();
}

// g++ -std=c++17 mainInterfaz.cpp recommendationSystem.cpp -o server \
//     -lpthread -lboost_system -I../include