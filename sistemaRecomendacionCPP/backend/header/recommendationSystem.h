#ifndef RECOMENDATION_SYSTEM_H
#define RECOMENDATION_SYSTEM_H

#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>
#include <thread>  // std::thread


using namespace std;
const float DISTANCIA_MAXIMA = 1e9;

// DEFINICION DE UMBRALES
const float UMBRAL_RATING_VECINO = 3.0;
const int UMBRAL_PELICULAS_COMUNES = 3;
const float UMBRAL_COSINE_SIMILARITY = 0.5; // Umbral para similitud del coseno
const float UMBRAL_PEARSON_CORRELATION = 0.5; // Umbral para correlación de Pearson
const int UMBRAL_VECINOS_SIMILARES = 0; // Umbral para el número de vecinos similares



class RecommendationSystem {
    private:
        unordered_map<int, unordered_map<int, float>> user_movie_ratings;
        unordered_set<int> users;
        unordered_map<int, pair<string, vector<string>>> movies;
        unordered_map<int, int> links;
        unordered_map<string, int> genres_map;

        ofstream cout_debug_file;
        ofstream cout_debug_file_01_validar_distancias,
                 cout_debug_file_02_calcular_knn,
                 cout_debug_file_03_calcular_recomendaciones,
                 cout_debug_file_04_peliculas_recomendar;
    public:
        RecommendationSystem();
		~RecommendationSystem();

        const std::unordered_set<int>& getUsers() const {
            return users;
        }
        const std::unordered_map<int, std::pair<std::string, std::vector<std::string>>>& getMovies() const {
            return movies;
        }
        const std::unordered_map<int, std::unordered_map<int, float>>& getUserMovieRatings() const {
            return user_movie_ratings;
        }
        const std::unordered_map<int, int>& getLinks() const {
            return links;
        }
        const std::unordered_map<std::string, int> &getGenresMap() const{
            return genres_map;
        }


        // implementar una funcion get para usar cout_debug_file_01_validar_distancias fuera de la clase
        ofstream& getCoutDebugFile(){
            return cout_debug_file;
        }
        ofstream& getCoutDebugFile01ValidarDistancias(){
            return cout_debug_file_01_validar_distancias;
        }
        ofstream& getCoutDebugFile02CalcularKNN(){
            return cout_debug_file_02_calcular_knn;
        }
        ofstream& getCoutDebugFile03CalcularRecomendaciones(){
            return cout_debug_file_03_calcular_recomendaciones;
        }
        ofstream& getCoutDebugFile04PeliculasRecomendar(){
            return cout_debug_file_04_peliculas_recomendar;
        }

        /*
			add to hash to hash
            addRatingAndUser(idUser, idMovie, rating, timestamp)
            idUser: int
            idMovie: int
            rating: float
            timestamp: string
		*/
		void addRatingAndUser(int, int, float, string);

        /*
            addMovie(idMovie, title, genres)
            idMovie: int
            title: string
            genres: vector<string>
        */
        void addMovie(int, const string, const vector<string>&);

        /*
            Funcion que retorna el numero de peliculas que ha calificado un usuario
            userId: int
            return: int
        */
        int getNumberOfRatedMovies(int);
        /*
			Calculo de la distancia euclidiana entre el usuario A y el usuario B
            userA: int
            userB: int
            commonMovies: int&
            return: pair<float, bool>
		*/        
		pair<float, bool> calculateEuclideanDistance(int, int, int &);
        pair<float, bool> calculateEuclideanDistanceDebug(int, int, int &);

        /*
            Calculo de la distancia de manhattan entre el usuario A y el usuario B
            userA: int
            userB: int
            commonMovies: int&
            return: pair<float, bool>
        */
       pair<float, bool> calculateManhattanDistance(int, int, int &);
       pair<float, bool> calculateManhattanDistanceDebug(int, int, int &);


       /*
        Calculo de la similitud del coseno entre el usuario A y el usuario B
        userA: int
        userB: int
        commonMovies: int&
        return: pair<float, bool>
       */
        pair<float, bool> calculateCosineSimilarity(int, int, int &);  
        pair<float, bool> calculateCosineSimilarityDebug(int, int, int &);      


        /*
            Calculo del coeficiente de correlacion de pearson entre el usuario A y el usuario B,
            mediante la aproximacion de pearson
            userA: int
            userB: int
            commonMovies: int&
            return: pair<float, bool>
        */
        pair<float, bool> calculatePearsonCorrelation(int, int, int &);
        pair<float, bool> calculatePearsonCorrelationDebug(int, int, int &);


        /*
            Funcion knn que calcula la distancia del usuarioX entre todos los usuarios
            y retorna un vector de n pares con el id del usuario y la distancia.
            n: int
            userX: int
            metric: string (puede ser "euclidean", "manhattan", "cosine", "pearson")
            return: vector<pair<int, float>>
            Esta funcion es utilizada para obtener las recomendaciones de un usuario.
        */
        vector<pair<int, float>> knn(int, int, string&);
        vector<pair<int, float>> knnParalelo(int, int, string);

        /*
            La funcion recomendar recibe lo que me retorno la funcio knn 
            y ademas al usuario que queremos recomendar.
            knn_result: vector<pair<int, float>>
            idUser: int 
        */
        unordered_map<int, vector<pair<float, int>>> recomendar(vector<pair<int, float>>&, int);
        unordered_map<int, vector<pair<float, int>>> recomendarDebug(vector<pair<int, float>>&, int);

        /*
            La funcion recomendarCancion retorna un vector de pares con el id de la cancion
            y su RATING CALCULADO.
            peliculasRecomendadasPorUsuarios: unordered_map<int,vector<pair<float, int>>>
            userARecomendar: int
        */
        vector<pair<float, int>> recomendarMovie(unordered_map<int, vector<pair<float, int>>> &, vector<pair<int, float>> &, int,string);
        vector<pair<float, int>> recomendarMovieDebug(unordered_map<int, vector<pair<float, int>>>&, vector<pair<int, float>>&, int, string);

        /*
            Agregar un usuaior
        */
        int addUser();

        /*
            CalificarPelicula
            Allow a user to rate multiple movies at once.
            idUser: ID of the user
            peliculas: List of movie IDs and their ratings
        */
        void calificarPeliculas(int, const vector<pair<int, float>>&);

        /*
            printUser()
            Prints the user information to out/users.txt
            This function is used for debugging purposes.
        */
        void printUser();
        void printLinks();

        /*
            printGenresFrequency()
            Prints the frequency of each genre in the system to out/genres_frequency.txt
            This function is used for debugging purposes.
            It iterates through the genres_map and prints the genre and its frequency.
        */
        void printGenresFrequency();

        /*
            Verifica si un usuario existe en el sistema
            userId: int
            return: bool
            Esta funcion es utilizada para verificar si un usuario existe en el sistema
            y es llamada desde la interfaz de usuario.
            Se usa en el endpoint /api/verify_user.
            Si el usuario existe, retorna true, de lo contrario false.
        */
        bool userExists(int);

        /*
            Esta funcion se encarga de recalificar una pelicula
            y se usa en el endpoint /api/recalificar_pelicula.
            idUser: int, idMovie: int, rating: float
            Esta funcion es utilizada para recalificar una pelicula
            y actualizar la calificacion del usuario en la pelicula.
            Si el usuario no ha calificado la pelicula, se agrega una nueva calificacion.
            Si el usuario ya ha calificado la pelicula, se actualiza la calificacion.
        */
        void recalificarPelicula(int, int, float);

        /*
            Esta funcion se encarga de validar las distancias entre los usuarios
            y las peliculas que han calificado.
            Se usa en el endpoint /api/validate_distances.
            Esta funcion es utilizada para verificar si las distancias entre los usuarios
            son correctas y si las peliculas que han calificado son correctas.
        */
        void validarDistancias();

        /*
            Esta funcion se encarga de calcular las recomendaciones de un usuario
            y se usa en el endpoint /api/recommendations.
        */
        void calcularRecomendaciones(int, const string&);

        /*
            Esta funcion se encarga de calcular las recomendaciones de un usuario
            y se usa en el endpoint /api/recommendations_parallel.
        */
        void calcularRecomendacionesParalelo(int, const string&);

        /*
            getUnratedMoviesByGenre
            Esta funcion retorna una lista de peliculas no calificadas por un usuario
            que pertenecen a un genero especifico.
            userId: int, genre: string, limit: int
            Esta funcion es utilizada para obtener las peliculas no calificadas por un usuario
            que pertenecen a un genero especifico.
            Se usa en el endpoint /api/unrated_movies_by_genre.
            userId: ID del usuario
            genre: Genero de la pelicula
            limit: Limite de peliculas a retornar (default 20)
            return: vector<tuple<int, string, vector<string>>>
        */
        vector<tuple<int, string, vector<string>>> getUnratedMoviesByGenre(int userId, const string& genre, int limit = 30);

        vector<pair<float, int>> getPopularMovies(int limit = 30);
        void addUserWithId(int userId);
};

#endif // RECOMENDATION_SYSTEM_H