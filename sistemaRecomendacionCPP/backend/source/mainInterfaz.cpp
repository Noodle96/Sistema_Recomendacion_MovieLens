#define CROW_MAIN
#define CROW_USE_BOOST
// #define CROW_USE_BOOST_ASIO


#include "crow.h"

#include "../header/recommendationSystem.h"
#include "../header/timer.h"

struct CORS {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        // Preflight request
        if (req.method == crow::HTTPMethod::OPTIONS) {
            cout  << "CORS Preflight request received" << endl;
            res.code = 204;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept, Origin");
            res.set_header("Access-Control-Max-Age", "86400");
            res.set_header("Vary", "Origin"); // Opcional, ayuda a proxies
            res.end();
            return;
        }

        // Para todas las demás peticiones
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept, Origin");
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept, Origin");
    }
};




int main() {
    // crow::SimpleApp app;
    crow::App<CORS> app;
    

    // 🔹 Instanciar el sistema de recomendación UNA sola vez
    RecommendationSystem sistema;
    // Esto ya carga ratings.csv, movies.csv y deja todo en RAM

    CROW_ROUTE(app, "/api/knn").methods("GET"_method)
    ([&sistema](const crow::request& req){
        crow::json::wvalue response;
        int user = 1, k = 5;
        std::string metric = "euclidean";

        if (req.url_params.get("user")) user = std::stoi(req.url_params.get("user"));
        if (req.url_params.get("k"))     k = std::stoi(req.url_params.get("k"));
        if (req.url_params.get("metric")) metric = req.url_params.get("metric");

        auto vecinos = sistema.knn(k, user, metric);
        for (int i = 0; i < vecinos.size(); ++i) {
            response[i]["id"] = vecinos[i].first;
            response[i]["score"] = vecinos[i].second;
        }

        return response;
    });







    CROW_ROUTE(app, "/api/add_user").methods("GET"_method)
    ([&sistema]() {
        crow::json::wvalue response;

        int nuevo_id = sistema.addUser();  // función modificada que retorna el ID
        response["user_id"] = nuevo_id;

        return crow::response(200, response);
    });










    // Puedes agregar más endpoints aquí: /api/recomendar, /api/user, etc.
    // http://localhost:8080/api/verify_user?user_id=175325
    CROW_ROUTE(app, "/api/verify_user").methods("GET"_method)
    ([&sistema](const crow::request& req){
        crow::json::wvalue response;

        // Leer el parámetro desde la URL
        const char* id_str = req.url_params.get("user_id");
        if (!id_str) {
            response["error"] = "Falta el parámetro user_id";
            return crow::response(400, response);
        }

        int user_id = std::stoi(id_str);
        
        // Verificar si el usuario existe
        bool existe = sistema.userExists(user_id);
        response["exists"] = existe;

        return crow::response(200, response);
    });











    // http://localhost:8080/api/user_ratings?user_id=5
    CROW_ROUTE(app, "/api/user_ratings").methods("GET"_method)([&](const crow::request& req) {
        crow::json::wvalue response;

        // Leer user_id del parámetro
        const char* user_id_str = req.url_params.get("user_id");
        if (!user_id_str) {
            response["error"] = "Missing user_id parameter";
            return crow::response(400, response);
        }

        int user_id = std::stoi(user_id_str);

        // Verificar si el usuario existe
        if (sistema.getUsers().find(user_id) == sistema.getUsers().end()) {
            response["error"] = "User not found";
            return crow::response(404, response);
        }


        const auto& ratings = sistema.getUserMovieRatings().at(user_id);
        const auto& movieInfo = sistema.getMovies();

        crow::json::wvalue arr = crow::json::wvalue::list();

        int i = 0;
        for (const auto& [movie_id, rating] : ratings) {
            if (movieInfo.find(movie_id) == movieInfo.end()) continue;
            int tmdbIdInt = sistema.getLinks().at(movie_id);

            const auto& [title, genres] = movieInfo.at(movie_id);
            arr[i]["movie_id"] = movie_id;
            arr[i]["title"] = title;
            arr[i]["rating"] = rating;
            arr[i]["tmdbId"] = tmdbIdInt;
            arr[i]["genres"] = crow::json::wvalue::list();
            for (size_t j = 0; j < genres.size(); ++j) {
                arr[i]["genres"][j] = genres[j];
            }
            i++;
        }
        return crow::response(200, arr);
    });


    CROW_ROUTE(app, "/api/calificar").methods("POST"_method)
    ([&sistema](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body || !body.has("user_id") || !body.has("movie_id") || !body.has("rating")) {
            return crow::response(400, "Faltan campos requeridos");
        }

        int user_id = body["user_id"].i();
        int movie_id = body["movie_id"].i();
        float rating = static_cast<float>(body["rating"].d());

        // Validar existencia de usuario
        if (sistema.getUsers().find(user_id) == sistema.getUsers().end()) {
            return crow::response(404, "Usuario no encontrado");
        }

        // Validar existencia de película
        if (sistema.getMovies().find(movie_id) == sistema.getMovies().end()) {
            return crow::response(404, "Película no encontrada");
        }

        sistema.recalificarPelicula(user_id, movie_id, rating);
        return crow::response(200, "Rating actualizado correctamente");
    });




    // http://localhost:8080/api/peliculas_no_calificadas?user_id=12&genre=Action
    CROW_ROUTE(app, "/api/peliculas_no_calificadas").methods("GET"_method)
    ([&sistema](const crow::request& req) {
        crow::json::wvalue response;

        const char* user_id_str = req.url_params.get("user_id");
        const char* genre_str = req.url_params.get("genre");

        if (!user_id_str || !genre_str) {
            response["error"] = "Parámetros 'user_id' y 'genre' son requeridos";
            return crow::response(400, response);
        }

        int user_id = std::stoi(user_id_str);
        string genre = genre_str;

        if (sistema.getUsers().find(user_id) == sistema.getUsers().end()) {
            response["error"] = "Usuario no encontrado";
            return crow::response(404, response);
        }

        auto peliculas = sistema.getUnratedMoviesByGenre(user_id, genre);

        crow::json::wvalue arr = crow::json::wvalue::list();
        int i = 0;
        for (const auto& [movieId, titulo, generos] : peliculas) {
            arr[i]["movie_id"] = movieId;
            arr[i]["title"] = titulo;
            arr[i]["genres"] = crow::json::wvalue::list();
            for (size_t j = 0; j < generos.size(); ++j) {
                arr[i]["genres"][j] = generos[j];
            }
            i++;
        }
        return crow::response(200, arr);
    });






    // http://localhost:8080/api/recomendar?user_id=12&n=5&metric=cosine
    CROW_ROUTE(app, "/api/recomendar").methods("GET"_method)
    ([&sistema](const crow::request& req) {
        crow::json::wvalue response;

        const char* user_id_str = req.url_params.get("user_id");
        const char* n_str = req.url_params.get("n");
        const char* metric_str = req.url_params.get("metric");

        if (!user_id_str || !n_str || !metric_str) {
            response["error"] = "Faltan parámetros requeridos (user_id, n, metric)";
            return crow::response(400, response);
        }

        int user_id = std::stoi(user_id_str);
        int n = std::stoi(n_str);
        std::string metric = metric_str;

        if (sistema.getUsers().find(user_id) == sistema.getUsers().end()) {
            response["error"] = "Usuario no encontrado";
            return crow::response(404, response);
        }

        ofstream& log4 = sistema.getCoutDebugFile04PeliculasRecomendar();
        log4 << "\t[KNN main] KNN+ + RECOMENDAR + RECOMENDARMOVIE begin" << endl;
        Timer timer("\t[KNN+ + RECOMENDAR + RECOMENDARMOVIE]");
        auto vecinos = sistema.knnParalelo(n, user_id, metric);
        if (vecinos.empty()) {
            response["error"] = "No se encontraron vecinos válidos";
            return crow::response(404, response);
        }

        auto recomendaciones_por_usuario = sistema.recomendar(vecinos, user_id);
        log4 << "\t";
        timer.printElapsed(log4, "seg");
        log4 << "\t[KNN main] knn+recomendarMovie END" << endl;
        auto finales = sistema.recomendarMovie(recomendaciones_por_usuario, user_id);
        log4 << "\t[KNN main] KNN+ + RECOMENDAR + RECOMENDARMOVIE end" << endl;

        const auto& movieInfo = sistema.getMovies();
        crow::json::wvalue result = crow::json::wvalue::list();

        int i = 0;
        for (const auto& [score, movie_id] : finales) {
            if (movieInfo.find(movie_id) == movieInfo.end()) continue;

            const auto& [title, genres] = movieInfo.at(movie_id);
            result[i]["movie_id"] = movie_id;
            result[i]["title"] = title;
            result[i]["score"] = score;
            result[i]["genres"] = crow::json::wvalue::list();

            for (size_t j = 0; j < genres.size(); ++j) {
                result[i]["genres"][j] = genres[j];
            }
            ++i;
        }
        return crow::response(200, result);
    });





    std::cout << "Servidor corriendo en http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
}
// with crow -Iinclude
//g++ -std=c++17 mainInterfaz.cpp recommendationSystem.cpp -o server -lpthread -lboost_system -I../include