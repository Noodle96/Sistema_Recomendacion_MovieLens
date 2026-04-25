#include "../header/recommendationSystem.h"
#include "../header/timer.h"

#include <fstream>
#include <sstream>
#include <set>
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN CONSTRUCTOR AND DESTRUCTOR
///////////////////////////////////////////////////////////////////////////////////////////////////////
RecommendationSystem::RecommendationSystem() {
	// Abre el archivo para escritura (sobrescribe cada vez)
	cout_debug_file.open("../out/output_recommendation_systema.txt", std::ios::out);
	cout_debug_file_01_validar_distancias.open("../out/01_validar_distancias.txt", std::ios::out);
	cout_debug_file_02_calcular_knn.open("../out/02_calcular_knn.txt", std::ios::out);
	cout_debug_file_03_calcular_recomendaciones.open("../out/03_calcular_recomendaciones.txt", std::ios::out);
	cout_debug_file_04_peliculas_recomendar.open("../out/04_peliculas_recomendar.txt", std::ios::out);
	if (!cout_debug_file) std::cerr << "No se pudo abrir debug.txt\n";
	if(!cout_debug_file_01_validar_distancias) std::cerr << "No se pudo abrir 01_validar_distancias.txt\n";
	if(!cout_debug_file_02_calcular_knn) std::cerr << "No se pudo abrir 02_calcular_knn.txt\n";
	if(!cout_debug_file_03_calcular_recomendaciones) std::cerr << "No se pudo abrir 03_calcular_recomendaciones.txt\n";
	if(!cout_debug_file_04_peliculas_recomendar) std::cerr << "No se pudo abrir 04_peliculas_recomendar.txt\n";

    string linea;
    int userId, movieId;
    float rating;
    string timestamp;
    // Constructor implementation can be empty or initialize data structures if needed
    cout_debug_file << "[RECOMMENDATION SYSTEM] RecommendationSystem()" << endl;
    cout_debug_file << "\t[RECOMMENDATION SYSTEM] Load ratings.csv BEGIN" << endl;
    Timer timer("Load ratings.csv");
    std::ifstream archivo_csv_ratings("../../../dataset/32M/ml-32m/ratings.csv");
    getline(archivo_csv_ratings,linea); // omitir la linea de cabecera
    cout_debug_file << "\t linea de cabecera: " << linea << "\n";
    // Verificar si el archivo se abrió correctamente
	if (archivo_csv_ratings.is_open()) {
		while (getline(archivo_csv_ratings, linea)) {
            stringstream ss(linea);
			string userIdStr, movieIdStr, ratingStr, timestampStr;
			getline(ss, userIdStr, ',');
			getline(ss, movieIdStr, ',');
			getline(ss, ratingStr, ',');
			getline(ss, timestampStr, ',');
			userId = stoi(userIdStr);
			movieId = stoi(movieIdStr);
			rating = stof(ratingStr);
			timestamp = timestampStr;
			addRatingAndUser(userId, movieId, rating, timestamp);
		}
		archivo_csv_ratings.close();
		cout_debug_file << "\t\t";
		timer.printElapsed(cout_debug_file);
    	cout_debug_file << "\t[RECOMMENDATION SYSTEM] Load ratings.csv END\n" << endl;
		cout_debug_file << "\t";
		printUser();
	} else {
		cout_debug_file << "\tError al abrir el archivo rating.csv" << std::endl;
	}
    cout_debug_file << "\t linea de cabecera: " << linea << "\n";


	string genre;
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] Load movies.csv BEGIN" << endl;
	timer.reset("Load movies.csv");
	std::ifstream archivo_csv_movies("../../../dataset/32M/ml-32m/movies.csv");
	getline(archivo_csv_movies, linea); // omitir la linea de cabecera
    cout_debug_file << "\t linea de cabecera: " << linea << "\n";
	// Verificar si el archivo se abrió correctamente
	if (archivo_csv_movies.is_open()) {
		while (getline(archivo_csv_movies, linea)) {
			string movieIdStr, title, genres;
			if (linea.find('"') != string::npos) {
				// Caso con comillas: id,"title, possibly with commas",genres
				size_t firstComma = linea.find(',');
				movieIdStr = linea.substr(0, firstComma);

				size_t firstQuote = linea.find('"', firstComma);
				size_t secondQuote = linea.find('"', firstQuote + 1);
				title = linea.substr(firstQuote + 1, secondQuote - firstQuote - 1);

				// Después de la segunda comilla + coma → vienen los géneros
				size_t genreStart = linea.find(',', secondQuote);
				genres = linea.substr(genreStart + 1);
			} else {
				// Caso sin comillas, separado normalmente
				stringstream ss(linea);
				getline(ss, movieIdStr, ',');
				getline(ss, title, ',');
				getline(ss, genres, ',');
			}

			// Conversión y almacenamiento
			int movieId = stoi(movieIdStr);
			vector<string> genreList;
			stringstream genreStream(genres);
			// cout_debug_file << "\tmovieId: " << movieId << ", title: " << title << ", genres: " << genres << endl;
			while (getline(genreStream, genre, '|')) {
				// Limpieza
				genre.erase(remove(genre.begin(), genre.end(), '\r'), genre.end());
				genre.erase(remove(genre.begin(), genre.end(), '\n'), genre.end());
				genre.erase(remove(genre.begin(), genre.end(), '\t'), genre.end());

				genreList.push_back(genre);
				genres_map[genre]++;
			}

			// Limpiar comillas residuales si quedan
			// title.erase(remove(title.begin(), title.end(), '\"'), title.end());

			addMovie(movieId, title, genreList);
		}

		archivo_csv_movies.close();
		cout_debug_file << "\t\t";
		timer.printElapsed(cout_debug_file);
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] Load movies.csv END\n" << endl;
	} else{
		cout_debug_file << "\tError al abrir el archivo movies.csv" << std::endl;
	}
	printGenresFrequency();

	// Agregando requerimineto links.csv
	std::ifstream archivo_csv_links("../../../dataset/32M/ml-32m/links.csv");
	getline(archivo_csv_links, linea); // omitir la linea de cabecera
	// Verificar si el archivo se abrió correctamente
	if (archivo_csv_links.is_open()) {
		while (getline(archivo_csv_links, linea)) {
			stringstream ss(linea);
			string movieIdStr, imdIDStr, tmdbIdStr;
			getline(ss, movieIdStr, ',');
			getline(ss, imdIDStr, ',');
			getline(ss, tmdbIdStr, ',');
			// cout_debug_file << "movieIdStr: " << movieIdStr << ", imdIDStr: " << imdIDStr << ", tmdbIdStr: " << tmdbIdStr << endl;
			int movieId = stoi(movieIdStr);
			int tmdbIdInt;
			tmdbIdStr.erase(std::remove_if(tmdbIdStr.begin(), tmdbIdStr.end(),
                [](unsigned char c) { return !std::isdigit(c); }),
                tmdbIdStr.end());

			if (!tmdbIdStr.empty()) {
				tmdbIdInt = stoi(tmdbIdStr);
			} else {
				tmdbIdInt = -1;
			}
			links[movieId] = tmdbIdInt;
		}
		archivo_csv_links.close();
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] Load links.csv END" << endl;
	} else {
		cout_debug_file << "\tError al abrir el archivo links.csv" << std::endl;
	}
	printLinks();
}

// Implementacion del destructor
RecommendationSystem::~RecommendationSystem() {
	if(cout_debug_file.is_open()) cout_debug_file.close();
	if(cout_debug_file_01_validar_distancias.is_open()) cout_debug_file_01_validar_distancias.close();
	if(cout_debug_file_02_calcular_knn.is_open()) cout_debug_file_02_calcular_knn.close();
	if(cout_debug_file_03_calcular_recomendaciones.is_open()) cout_debug_file_03_calcular_recomendaciones.close();
	if(cout_debug_file_04_peliculas_recomendar.is_open()) cout_debug_file_04_peliculas_recomendar.close();
	cout_debug_file << "[RECOMMENDATION SYSTEM] Destructor called" << endl;																																																																																																			
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END CONSTRUCTOR AND DESTRUCTOR
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN ADD FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////
void RecommendationSystem::addRatingAndUser(int userId, int movieId, float rating, string timestamp){
	user_movie_ratings[userId][movieId] = rating;
	users.insert(userId);
}

void RecommendationSystem::addMovie(int movieId, const string title, const vector<string>& genres) {
	movies[movieId] = make_pair(title, genres);
}
int RecommendationSystem::getNumberOfRatedMovies(int userId) {
	if (user_movie_ratings.find(userId) != user_movie_ratings.end()) {
		return user_movie_ratings[userId].size();
	}
	return 0; // Usuario no encontrado o sin calificaciones
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END ADD FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN EUCLIDEAN DISTANCE
///////////////////////////////////////////////////////////////////////////////////////////////////////
pair<float, bool> RecommendationSystem::calculateEuclideanDistance(int userA, int userB, int &commonMovies){
	float euclideanDistance = 0.0;
	commonMovies = 0;
	// usuarios validos
	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if(hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()){
		// cout << DEVELOPING <<"UserA or UserB not found" << endl;
		// retornar DISTANCIA_MAXIMA ya que no hay peliculas en comun
		// usuario no valido o no tiene calificaciones
		return {DISTANCIA_MAXIMA,false};
	}
	/*
		* Es eficiente comparar quien tiene menos peliculas recomendadas contra el que tiene mas peliculas
		* Para ello condicionamos con el criterio anterior
	*/
	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
    const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	for (const auto& [movie, rating] : smaller) {
        auto it = larger.find(movie);
        if (it != larger.end()) {
            float diff = rating - it->second;
            euclideanDistance += diff * diff;
            ++commonMovies;
        }
    }
	if (commonMovies > 0)
        return {sqrt(euclideanDistance), true};
    return {DISTANCIA_MAXIMA, false};
}

pair<float, bool> RecommendationSystem::calculateEuclideanDistanceDebug(int userA, int userB, int &commonMovies){
	cout_debug_file_01_validar_distancias << "[EUCLIDEAN DISTANCE] calculateEuclideanDistanceDebug() BEGIN" << endl;
	cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] distance between " << userA << " and " << userB << endl;

	float euclideanDistance = 0.0;
	commonMovies = 0;
	// usuarios validos
	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if(hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()){
		// cout << DEVELOPING <<"UserA or UserB not found" << endl;
		// retornar DISTANCIA_MAXIMA ya que no hay peliculas en comun
		// usuario no valido o no tiene calificaciones
		cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] usuario no valido o no tiene ratings" << endl;
		cout_debug_file_01_validar_distancias << "[EUCLIDEAN DISTANCE] calculateEuclideanDistanceDebug() END\n\n" << endl;
		return {DISTANCIA_MAXIMA,false};
	}
	/*
		* Es eficiente comparar quien tiene menos peliculas recomendadas contra el que tiene mas peliculas
		* Para ello condicionamos con el criterio anterior
	*/
	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
    const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] smaller size: " << smaller.size() << endl;
	cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] larger size: " << larger.size() << endl;

	Timer timer("Calculate Euclidean Distance");
	for (const auto& [movie, rating] : smaller) {
        auto it = larger.find(movie);
        if (it != larger.end()) {
            float diff = rating - it->second;
            euclideanDistance += diff * diff;
            ++commonMovies;
        }
    }
	cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] commonMovies: " << commonMovies << endl;
	cout_debug_file_01_validar_distancias << "\t";
	timer.printElapsed(cout_debug_file_01_validar_distancias, "seg");
	if (commonMovies > 0){
		cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] euclidean distance: " << fixed << setprecision(8)<< sqrt(euclideanDistance) << endl;
		cout_debug_file_01_validar_distancias << "[EUCLIDEAN DISTANCE] calculateEuclideanDistanceDebug() END\n\n" << endl;
		return {sqrt(euclideanDistance), true};
	}
	else{
		cout_debug_file_01_validar_distancias << "\t[EUCLIDEAN DISTANCE] No common movies found between user " << userA << " and user " << userB << endl;
		cout_debug_file_01_validar_distancias << "[EUCLIDEAN DISTANCE] calculateEuclideanDistanceDebug() END\n\n" << endl;
		return {DISTANCIA_MAXIMA, false};
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END EUCLIDEAN DISTANCE
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN MANHATTAN DISTANCE
///////////////////////////////////////////////////////////////////////////////////////////////////////
pair<float, bool> RecommendationSystem::calculateManhattanDistance(int userA, int userB, int &commonMovies){
	float manhattanDistance = 0.0;
	commonMovies = 0;
	// usuarios validos
	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if(hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()){
		// cout << DEVELOPING <<"UserA or UserB not found" << endl;
		// retornar DISTANCIA_MAXIMA ya que no hay peliculas en comun
		// usuario no valido o no tiene calificaciones
		return {DISTANCIA_MAXIMA,false};
	}
	/*
		* Es eficiente comparar quien tiene menos peliculas recomendadas contra el que tiene mas peliculas
		* Para ello condicionamos con el criterio anterior
	*/
	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
    const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	for (const auto& [movie, rating] : smaller) {
        auto it = larger.find(movie);
        if (it != larger.end()) {
            float diff = rating - it->second;
            manhattanDistance += abs(diff);
            ++commonMovies;
        }
    }
	if (commonMovies > 0)
        return {manhattanDistance, true};
    return {DISTANCIA_MAXIMA, false};
}
pair<float, bool> RecommendationSystem::calculateManhattanDistanceDebug(int userA, int userB, int &commonMovies){
	cout_debug_file_01_validar_distancias << "[MANHATTAN DISTANCE] calculateManhattanDistanceDebug() BEGIN" << endl;
	cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] distance between " << userA << " and " << userB << endl;
	float manhattanDistance = 0.0;
	commonMovies = 0;
	// usuarios validos
	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if(hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()){
		// cout << DEVELOPING <<"UserA or UserB not found" << endl;
		// retornar DISTANCIA_MAXIMA ya que no hay peliculas en comun
		// usuario no valido o no tiene calificaciones
		cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] usuario no valido o no tiene ratings" << endl;
		cout_debug_file_01_validar_distancias << "[MANHATTAN DISTANCE] calculateManhattanDistanceDebug() END\n\n" << endl;
		// retornar DISTANCIA_MAXIMA
		return {DISTANCIA_MAXIMA,false};
	}
	/*
		* Es eficiente comparar quien tiene menos peliculas recomendadas contra el que tiene mas peliculas
		* Para ello condicionamos con el criterio anterior
	*/
	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
    const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] smaller size: " << smaller.size() << endl;
	cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] larger size: " << larger.size() << endl;

	Timer timer("calculate Manhattan Distance");
	for (const auto& [movie, rating] : smaller) {
        auto it = larger.find(movie);
        if (it != larger.end()) {
            float diff = rating - it->second;
            manhattanDistance += abs(diff);
            ++commonMovies;
        }
    }
	cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] commonMovies: " << commonMovies << endl;
	cout_debug_file_01_validar_distancias << "\t";
	timer.printElapsed(cout_debug_file_01_validar_distancias, "seg");
	if (commonMovies > 0){
		cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] manhattan distance: " << fixed << setprecision(8)<< manhattanDistance << endl;
		cout_debug_file_01_validar_distancias << "[MANHATTAN DISTANCE] calculateManhattanDistanceDebug() END\n\n" << endl;
    	return {manhattanDistance, true};
	}else{
		cout_debug_file_01_validar_distancias << "\t[MANHATTAN DISTANCE] No common movies found between user " << userA << " and user " << userB << endl;
		cout_debug_file_01_validar_distancias << "[MANHATTAN DISTANCE] calculateManhattanDistanceDebug() END\n\n" << endl;
		return {DISTANCIA_MAXIMA, false};
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END MANHATTAN DISTANCE
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN COSINE SIMILARITY
///////////////////////////////////////////////////////////////////////////////////////////////////////
pair<float, bool> RecommendationSystem::calculateCosineSimilarity(int userA, int userB, int &commonMovies) {
	// Implementación de la similitud del coseno
	float dotProduct = 0.0;
	float normA = 0.0;
	float normB = 0.0;
	commonMovies = 0;

	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];	

	if (hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()) {
		// POdemos asignar tambien -1, pero al tener interseccion como false, solamente no las vamos a considar
		return {0.0, false}; // el usuario no tiene registros de calificacion o usuario no existe
	}

	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;

	for (const auto& [movie, ratingA] : smaller) {
		auto it = larger.find(movie);
		if (it != larger.end()) {
			float ratingB = it->second;
			dotProduct += ratingA * ratingB;
			normA += ratingA * ratingA;
			normB += ratingB * ratingB;
			++commonMovies;
		}
	}

	if (commonMovies == 0) {
		return {0.0, false}; // No hay películas en común
	}
	if( normA == 0 || normB == 0) {
		return {0.0, false}; // Evitar división por cero
	}
	float cosineSimilarity = dotProduct / (sqrt(normA) * sqrt(normB));
	return {cosineSimilarity, true}; // Retorna la similitud del coseno y true
}
pair<float, bool> RecommendationSystem::calculateCosineSimilarityDebug(int userA, int userB, int &commonMovies) {
	cout_debug_file_01_validar_distancias << "[COSINE SIMILARITY] calculateCosineSimilarityDebug() BEGIN" << endl;
	cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] similarity between " << userA << " and " << userB << endl;
	// Implementación de la similitud del coseno
	float dotProduct = 0.0;
	float normA = 0.0;
	float normB = 0.0;
	commonMovies = 0;

	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];	

	if (hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()) {
		// POdemos asignar tambien -1, pero al tener interseccion como false, solamente no las vamos a considar
		cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] usuario no valido o no tiene ratings" << endl;
		cout_debug_file_01_validar_distancias << "[COSINE SIMILARITY] calculateCosineSimilarityDebug() END\n\n" << endl;
		return {0.0, false}; // el usuario no tiene registros de calificacion o usuario no existe
	}

	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] smaller size: " << smaller.size() << endl;
	cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] larger size: " << larger.size() << endl;

	Timer timer("Calculate Cosine Similarity");
	for (const auto& [movie, ratingA] : smaller) {
		auto it = larger.find(movie);
		if (it != larger.end()) {
			float ratingB = it->second;
			dotProduct += ratingA * ratingB;
			normA += ratingA * ratingA;
			normB += ratingB * ratingB;
			++commonMovies;
		}
	}
	cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] commonMovies: " << commonMovies << endl;
	cout_debug_file_01_validar_distancias << "\t";
	timer.printElapsed(cout_debug_file_01_validar_distancias, "seg");

	if (commonMovies == 0) {
		cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] No common movies found between user " << userA << " and user " << userB << endl;
		cout_debug_file_01_validar_distancias << "[COSINE SIMILARITY] calculateCosineSimilarityDebug() END\n\n" << endl;
		// No hay películas en
		return {0.0, false}; // No hay películas en común
	}
	if( normA == 0 || normB == 0){
		cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] normA or normB is zero, returning 0.0" << endl;
		cout_debug_file_01_validar_distancias << "[COSINE SIMILARITY] calculateCosineSimilarityDebug() END\n\n" << endl;
		return {0.0, false}; // Evitar división por cero
	}
	float cosineSimilarity = dotProduct / (sqrt(normA) * sqrt(normB));
	cout_debug_file_01_validar_distancias << "\t[COSINE SIMILARITY] cosine similarity: "<< fixed << setprecision(8) << cosineSimilarity << endl;
	cout_debug_file_01_validar_distancias << "[COSINE SIMILARITY] calculateCosineSimilarityDebug() END\n\n" << endl;
	return {cosineSimilarity, true}; // Retorna la similitud del coseno y true
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END COSINE SIMILARITY
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											BEGIN PEARSON CORRELATION
///////////////////////////////////////////////////////////////////////////////////////////////////////
pair<float, bool> RecommendationSystem::calculatePearsonCorrelation(int userA, int userB, int &commonMovies){
	float sumA = 0.0, sumB = 0.0, sumA2 = 0.0, sumB2 = 0.0, sumAB = 0.0;
	commonMovies = 0;

	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if (hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()) {
		return {0.0, false}; // el usuario no tiene registros de calificacion o usuario no valido
	}

	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;

	for (const auto& [movie, ratingA] : smaller) {
		auto it = larger.find(movie);
		if (it != larger.end()) {
			float ratingB = it->second;
			sumA += ratingA;
			sumB += ratingB;
			sumA2 += ratingA * ratingA;
			sumB2 += ratingB * ratingB;
			sumAB += ratingA * ratingB;
			++commonMovies;
		}
	}

	if (commonMovies == 0) {
		return {0.0, false}; // No hay películas en común
	}

	float numerator = sumAB - (sumA * sumB / commonMovies);
	float denominator = sqrt((sumA2 - (sumA * sumA / commonMovies))) * sqrt((sumB2 - (sumB * sumB / commonMovies)));

	if (denominator == 0) {
		return {0.0, false}; // Evitar división por cero
	}

	float pearsonCorrelation = numerator / denominator;
	return {pearsonCorrelation, true}; // Retorna el coeficiente de correlación de Pearson y true
}
pair<float, bool> RecommendationSystem::calculatePearsonCorrelationDebug(int userA, int userB, int &commonMovies){
	cout_debug_file_01_validar_distancias << "[PEARSON CORRELATION] calculatePearsonCorrelationDebug() BEGIN" << endl;
	cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] correlation between " << userA << " and " << userB << endl;
	float sumA = 0.0, sumB = 0.0, sumA2 = 0.0, sumB2 = 0.0, sumAB = 0.0;
	commonMovies = 0;

	const auto& hash_movie_rating_userA = user_movie_ratings[userA];
	const auto& hash_movie_rating_userB = user_movie_ratings[userB];

	if (hash_movie_rating_userA.empty() || hash_movie_rating_userB.empty()) {
		cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] usuario no valido o no tiene ratings" << endl;
		cout_debug_file_01_validar_distancias << "[PEARSON CORRELATION] calculatePearsonCorrelationDebug() END\n\n" << endl;
		return {0.0, false}; // el usuario no tiene registros de calificacion o usuario no valido
	}

	const auto& smaller = (hash_movie_rating_userA.size() <= hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	const auto& larger  = (hash_movie_rating_userA.size() >  hash_movie_rating_userB.size()) ? hash_movie_rating_userA : hash_movie_rating_userB;
	cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] smaller size: " << smaller.size() << endl;
	cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] larger size: " << larger.size() << endl;

	Timer timer("Calculate Pearson Correlation");
	for (const auto& [movie, ratingA] : smaller) {
		auto it = larger.find(movie);
		if (it != larger.end()) {
			float ratingB = it->second;
			sumA += ratingA;
			sumB += ratingB;
			sumA2 += ratingA * ratingA;
			sumB2 += ratingB * ratingB;
			sumAB += ratingA * ratingB;
			++commonMovies;
		}
	}
	cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] commonMovies: " << commonMovies << endl;
	cout_debug_file_01_validar_distancias << "\t";
	timer.printElapsed(cout_debug_file_01_validar_distancias, "seg");

	if (commonMovies == 0) {
		cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] No common movies found between user " << userA << " and user " << userB << endl;
		cout_debug_file_01_validar_distancias << "[PEARSON CORRELATION] calculatePearsonCorrelationDebug() END\n\n" << endl;
		return {0.0, false}; // No hay películas en común
	}

	float numerator = sumAB - (sumA * sumB / commonMovies);
	float denominator = sqrt((sumA2 - (sumA * sumA / commonMovies))) * sqrt((sumB2 - (sumB * sumB / commonMovies)));

	if (denominator == 0) {
		cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] denominator is zero, returning 0.0" << endl;
		cout_debug_file_01_validar_distancias << "[PEARSON CORRELATION] calculatePearsonCorrelationDebug() END\n\n" << endl;
		return {0.0, false}; // Evitar división por cero
	}

	float pearsonCorrelation = numerator / denominator;
	cout_debug_file_01_validar_distancias << "\t[PEARSON CORRELATION] pearson correlation: "<< fixed << setprecision(8) << pearsonCorrelation << endl;
	cout_debug_file_01_validar_distancias << "[PEARSON CORRELATION] calculatePearsonCorrelationDebug() END\n\n" << endl;
	return {pearsonCorrelation, true}; // Retorna el coeficiente de correlación de Pearson y true
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END PEARSON CORRELATION
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 										    	BEGIN KNN 02
///////////////////////////////////////////////////////////////////////////////////////////////////////
vector<pair<int, float>> RecommendationSystem::knn(int n, int userX, string& metrica){
	vector<pair<int, float>> distances;
	int commonMovies = 0;

	for (const auto& user : users) {
		if (user != userX) {
			pair<float, bool> distance;
			if (metrica == "euclidean") {
				distance = calculateEuclideanDistance(userX, user, commonMovies);
			} else if (metrica == "manhattan") {
				distance = calculateManhattanDistance(userX, user, commonMovies);
			} else if (metrica == "cosine") {
				distance = calculateCosineSimilarity(userX, user, commonMovies);
				if(distance.first < UMBRAL_COSINE_SIMILARITY) continue;
			} else if (metrica == "pearson") {
				distance = calculatePearsonCorrelation(userX, user, commonMovies);
				if(distance.first < UMBRAL_PEARSON_CORRELATION) continue;
			} else {
				cout_debug_file << "Metrica no valida" << endl;
				return {};
			}
			// Si hay peliculas en comun y sea mayor al umbral
			if (distance.second && commonMovies >= UMBRAL_PELICULAS_COMUNES) { 
				distances.emplace_back(user, distance.first);
			}
		}
	}
	if(metrica == "euclidean" || metrica == "manhattan") {
		// Si la metrica es euclidean o manhattan, ordenamos por distancia ascendente
		sort(distances.begin(), distances.end(), [](const pair<int, float>& a, const pair<int, float>& b) {
			return a.second < b.second; // Ordenar por distancia ascendente
		});
	} else if (metrica == "cosine" || metrica == "pearson") {
		// Si la metrica es cosine o pearson, ordenamos por similitud descendente
		sort(distances.begin(), distances.end(), [](const pair<int, float>& a, const pair<int, float>& b) {
			return a.second > b.second; // Ordenar por similitud descendente
		});
	}

	if (distances.size() > n) {
		distances.resize(n); // Limitar a los n vecinos más cercanos
	}
	return distances;
}

vector<pair<int, float>> RecommendationSystem::knnParalelo(int n, int userX, string metrica){
    vector<pair<int, float>> final_distances;
    int num_threads = thread::hardware_concurrency();
	cout << "num:threads: " << num_threads << endl;
    if (num_threads == 0) num_threads = 4; // valor por defecto si no se detecta

    // Copiar usuarios a un vector para indexado fácil
    vector<int> all_users;
    for (const auto& user : users) {
        if (user != userX) all_users.push_back(user);
    }

    size_t total_users = all_users.size();
    size_t block_size = (total_users + num_threads - 1) / num_threads;

    // Cada hilo tendrá su vector local
    vector<vector<pair<int, float>>> partial_results(num_threads);
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        size_t start = t * block_size;
        size_t end = min(start + block_size, total_users);

        threads.emplace_back([&, t, start, end]() {
            for (size_t i = start; i < end; ++i) {
                int commonMovies = 0;
                int userY = all_users[i];
                pair<float, bool> distance;

                if (metrica == "euclidean") {
                    distance = calculateEuclideanDistance(userX, userY, commonMovies);
                } else if (metrica == "manhattan") {
                    distance = calculateManhattanDistance(userX, userY, commonMovies);
                } else if (metrica == "cosine") {
                    distance = calculateCosineSimilarity(userX, userY, commonMovies);
                    if (distance.first < UMBRAL_COSINE_SIMILARITY) continue;
                } else if (metrica == "pearson") {
                    distance = calculatePearsonCorrelation(userX, userY, commonMovies);
                    if (distance.first < UMBRAL_PEARSON_CORRELATION) continue;
                } else {
                    // métrica no válida
                    continue;
                }

                if (distance.second && commonMovies >= UMBRAL_PELICULAS_COMUNES) {
                    partial_results[t].emplace_back(userY, distance.first);
                }
            }
        });
    }

    // Esperar a que todos los hilos terminen
    for (auto& th : threads) th.join();

    // Fusionar resultados
    for (const auto& local_vec : partial_results) {
        final_distances.insert(final_distances.end(), local_vec.begin(), local_vec.end());
    }

    // Ordenar
    if (metrica == "euclidean" || metrica == "manhattan") {
        sort(final_distances.begin(), final_distances.end(),
            [](const pair<int, float>& a, const pair<int, float>& b) {
                return a.second < b.second;
            });
    } else if (metrica == "cosine" || metrica == "pearson") {
        sort(final_distances.begin(), final_distances.end(),
            [](const pair<int, float>& a, const pair<int, float>& b) {
                return a.second > b.second;
            });
    }

    if ((int)final_distances.size() > n) {
        final_distances.resize(n);
    }

    return final_distances;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 												  END KNN 02
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 												BEGIN RECOMENDAR 03
///////////////////////////////////////////////////////////////////////////////////////////////////////
unordered_map<int, vector<pair<float, int>>> RecommendationSystem::recomendar(vector<pair<int, float>>& knn_result, int userARecomendar){
    // Significa que el user "Y" le recomienda al usuario "userARecomendar" con [rating "X" la pelicula "Z"]
	unordered_map<int, vector<pair<float, int>>> recommended_movies; // Peliculas recomendadas y sus ratings
	if(users.find(userARecomendar) == users.end()){
		return recommended_movies; // Usuario no encontrado
	}
	auto& hash_movie_rating_userARecomendar = user_movie_ratings[userARecomendar];
	for(auto & [userX, distance] : knn_result){
		auto& hash_movie_rating_userX = user_movie_ratings[userX];
		// debemos de recomendar las peliculas que el usuarioX halla visto y el usuarioARecomendar no haya visto
		for(const auto& [movie, rating] : hash_movie_rating_userX){
			if(rating < UMBRAL_RATING_VECINO) continue; // Si la calificacion es menor a 3, no la recomendamos
			auto movieFound = hash_movie_rating_userARecomendar.find(movie);
			// este if dice que la pelicula no ha sido vista por el usuarioARecomendar
			if( movieFound == hash_movie_rating_userARecomendar.end() ){
				// si la pelicula no esta en el hash de ratings del usuarioARecomendar, la recomendamos
				auto& movieInfo = this->movies[movie];
				// recommended_movies[userX].emplace_back(make_pair(rating,movieInfo.first));
				recommended_movies[userX].emplace_back(make_pair(rating,movie));

			}
		}
		// ordenamos el valor del hash
		sort(recommended_movies[userX].begin(), recommended_movies[userX].end(), [](const pair<float, int>& a, const pair<float, int>& b) {
			return a.first > b.first; // Ordenar por rating descendente
		});
	}
	return recommended_movies;
}
unordered_map<int, vector<pair<float, int>>> RecommendationSystem::recomendarDebug(vector<pair<int, float>>& knn_result, int userARecomendar){
	unordered_map<int, vector<pair<float, int>>> recommended_movies; // Peliculas recomendadas y sus ratings

	cout_debug_file_03_calcular_recomendaciones << "[RECOMENDAR] recomendar() BEGIN" << endl;
	if(users.find(userARecomendar) == users.end()){
		cout_debug_file_03_calcular_recomendaciones << "\t[RECOMENDAR] User " << userARecomendar << " not found" << endl;
		return recommended_movies; // Usuario no encontrado
	}
	// unordered_map<int, vector<pair<float, string>>> recommended_movies; // Peliculas recomendadas y sus rating
	auto& hash_movie_rating_userARecomendar = user_movie_ratings[userARecomendar];

	Timer timer("first timer");
	for(auto & [userX, distance] : knn_result){
		auto& hash_movie_rating_userX = user_movie_ratings[userX];
		// debemos de recomendar las peliculas que el usuarioX halla visto y el usuarioARecomendar no haya visto
		for(const auto& [movie, rating] : hash_movie_rating_userX){
			if(rating < UMBRAL_RATING_VECINO) continue; // Si la calificacion es menor a 3, no la recomendamos
			auto movieFound = hash_movie_rating_userARecomendar.find(movie);
			// este if dice que la pelicula no ha sido vista por el usuarioARecomendar
			if( movieFound == hash_movie_rating_userARecomendar.end() ){
				// si la pelicula no esta en el hash de ratings del usuarioARecomendar, la recomendamos
				auto& movieInfo = this->movies[movie];
				// recommended_movies[userX].emplace_back(make_pair(rating,movieInfo.first));
				recommended_movies[userX].emplace_back(make_pair(rating,movie));

			}
		}
		// ordenamos el valor del hash
		sort(recommended_movies[userX].begin(), recommended_movies[userX].end(), [](const pair<float, int>& a, const pair<float, int>& b) {
			return a.first > b.first; // Ordenar por rating descendente
		});
	}
	cout_debug_file_03_calcular_recomendaciones << "\t[RECOMENDAR] Time taken to process recommendations: ";
	timer.printElapsed(cout_debug_file_03_calcular_recomendaciones, "seg");


	cout_debug_file_03_calcular_recomendaciones << "\t[RECOMENDAR] Recommended movies for user " << userARecomendar << ":" << endl;
	if(recommended_movies.empty()){
		cout_debug_file_03_calcular_recomendaciones << "\t[RECOMENDAR] No recommendations found for user " << userARecomendar << endl;
	}
	cout_debug_file_03_calcular_recomendaciones << "\t\t[RECOMENDAR] Peliculas calificadas por el usuario " << userARecomendar << " = " << this->getNumberOfRatedMovies(userARecomendar)<< endl;
	int vecino = 1;
	// Timer timer2("second timer");
	timer.reset();
	int contadorTest = 0;
	for (const auto& [userX, movies] : recommended_movies) {
		cout_debug_file_03_calcular_recomendaciones << "\t\t[RECOMENDAR] Recomendaciones del User " << userX << "[" << this->getNumberOfRatedMovies(userX) << "]"
		<<" vecino #"<< vecino++ << ", total movies: " << movies.size()<<endl;
		for (const auto& [movieRating, movieTitle] : movies) {
			cout_debug_file_03_calcular_recomendaciones << "\t\t\t[RECOMENDAR] Movie: " << movieTitle << " with rating: " << fixed << setprecision(1) << movieRating << endl;
		}
		cout_debug_file_03_calcular_recomendaciones << "\n";
		contadorTest += 1;
		if(contadorTest >= 20) break;
	}
	cout_debug_file_03_calcular_recomendaciones << "\t[RECOMENDAR] Time taken to process recommendations: ";
	timer.printElapsed(cout_debug_file_03_calcular_recomendaciones, "seg");
	cout_debug_file_03_calcular_recomendaciones << "[RECOMENDAR] recomendar() END" << endl;
	return recommended_movies;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 												END RECOMENDAR 03
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 										BEGIN RECOMENDATION SYSTEM 04
///////////////////////////////////////////////////////////////////////////////////////////////////////
vector<pair<float, int>> RecommendationSystem::recomendarMovie(unordered_map<int,vector<pair<float, int>>>& peliculasRecomendadasByUser, int userARecomendar){
	// recommended_movies[userX].emplace_back(make_pair(rating,movie));
	cout_debug_file_04_peliculas_recomendar << "[RECOMENDAR MOVIE] recomendarCancion() BEGIN" << endl;
	cout_debug_file_04_peliculas_recomendar << "\t[RECOMENDAR MOVIE] Recomendacion para el user: " << userARecomendar << endl;
	Timer timer("recomendarMovie");
	unordered_map<int, vector<float>> movie_vectorRatings;
	for (const auto& [user, movies] : peliculasRecomendadasByUser) {
		for (const auto& [rating, movieId] : movies) {
			movie_vectorRatings[movieId].push_back(rating);
		}
	}
	// cout_debug_file_04_peliculas_recomendar << "\t";
	// timer.printElapsed(cout_debug_file_04_peliculas_recomendar, "seg");

	vector<pair<float, int>> respuestaFinal;
	// Timer timer2("calculo all score");
	for(const auto&[movieId, ratings]: movie_vectorRatings){
		float suma = 0;
		for(auto rating: ratings) suma += rating;
		int count = (int)ratings.size();
		if(count < UMBRAL_VECINOS_SIMILARES) continue; // Si hay menos de 3 vecinos, no recomendamos
		// Calcular el score final
		int totalVecinos = (int)peliculasRecomendadasByUser.size();
		float score = (suma*count)/ totalVecinos;
		respuestaFinal.emplace_back(score, movieId);
	}
	sort(respuestaFinal.begin(), respuestaFinal.end(), [](const pair<float, int>& a, const pair<float, int>& b) {
		return a.first > b.first; // Ordenar por score descendente
	});
	cout_debug_file_04_peliculas_recomendar << "\t";
	timer.printElapsed(cout_debug_file_04_peliculas_recomendar,"seg");

	Timer timer3("write recomendacion movies");
	cout_debug_file_04_peliculas_recomendar << "\t[RECOMENDAR MOVIE] write recomendacion movies" << endl;
	for(const auto& [scoreCalculado, movieID]: respuestaFinal){
		cout_debug_file_04_peliculas_recomendar << "\t\t[RECOMENDAR MOVIE] Movie ID: " << movieID << "{" << movies[movieID].first << "}"
		<< " with score: " << fixed << setprecision(4) << scoreCalculado << endl;
	}
	cout_debug_file_04_peliculas_recomendar << "\t";
	timer3.printElapsed(cout_debug_file_04_peliculas_recomendar);
	cout_debug_file_04_peliculas_recomendar << "[RECOMENDAR MOVIE] recomendarCancion() END" << endl;
	return respuestaFinal;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 										END RECOMENDATION SYSTEM 04
///////////////////////////////////////////////////////////////////////////////////////////////////////
//													|
//										            |
//													|
//													|
//													|
//													|
//													|
// 												    |
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 												BEGIN OTHERS
///////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    Agregar un usuario
*/
int RecommendationSystem::addUser(){
	cout_debug_file << "[RECOMMENDATION SYSTEM] addUser() BEGIN" << endl;
	int new_user_id = users.size() + 1; // Asignar un nuevo ID de usuario
	if(users.find(new_user_id) != users.end()){
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << new_user_id << " already exists." << endl;
		return -1;
	}
	users.insert(new_user_id);
	user_movie_ratings[new_user_id] = unordered_map<int, float>(); // Inicializar con un hash vacío
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << new_user_id << " added successfully." << endl;
	cout_debug_file << "[RECOMMENDATION SYSTEM] addUser() END\n" << endl;
	printUser(); // Print the user ratings after adding
	return new_user_id; // Retornar el ID del nuevo usuario
}
/*
	CalificarPelicula
	Allow a user to rate multiple movies at once.
	idUser: ID of the user
	peliculas: List of movie IDs and their ratings
*/
void RecommendationSystem::calificarPeliculas(int idUser, const vector<pair<int, float>>& peliculas){
	cout_debug_file << "[RECOMMENDATION SYSTEM] calificarPeliculas() BEGIN" << endl;
	if(users.find(idUser) == users.end()){
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << idUser << " not found." << endl;
		return;
	}
	for(const auto& [movieId, rating] : peliculas){
		if(movies.find(movieId) == movies.end()){
			cout_debug_file << "\t[RECOMMENDATION SYSTEM] Movie ID " << movieId << " not found." << endl;
			continue; // Skip if movie does not exist
		}
		user_movie_ratings[idUser][movieId] = rating; // Add or update the rating
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << idUser << " rated movie ID " << movieId 
						<< " with rating: " << fixed << setprecision(1) << rating << endl;
	}
	cout_debug_file << "[RECOMMENDATION SYSTEM] calificarPeliculas() END\n" << endl;
	printUser(); // Print the user ratings after adding
}
void RecommendationSystem::printUser() {
	cout_debug_file << "[RECOMMENDATION SYSTEM] printUser() BEGIN" << endl;
    Timer timer("write users.txt");
	ofstream user_file("../out/users.txt");
	if (!user_file) {
		cout_debug_file << "No se pudo abrir el archivo users.txt\n";
		return;
	}
	int maximo = 0;
    std::set<pair<int,int> > setRatings;
	for (const auto& user : users) {
		int size = user_movie_ratings[user].size();
        setRatings.insert(make_pair(size, user));
		maximo = max(maximo, size);
		// user_file << "User ID: "<<user << " -> " << user_movie_ratings[user].size() << " ratings\n";
	}
    for(auto &[size, user]: setRatings){
        user_file << "User ID: " << user << " -> " << size << " ratings\n";
    }
	cout_debug_file << "\t[PRINTUSER] Usuario con maximo peliculas calificadas: " << maximo << endl;
	user_file.close();
	cout_debug_file << "\t\t";
	timer.printElapsed(cout_debug_file);
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] printUser() END\n" << endl;
}

void RecommendationSystem::printLinks(){
	cout_debug_file << "[RECOMMENDATION SYSTEM] printLinks() BEGIN" << endl;
	Timer timer("write links.txt");
	ofstream links_file("../out/links.txt");
	if (!links_file) {
		cout_debug_file << "No se pudo abrir el archivo links.txt\n";
		return;
	}
	for (const auto& [movieId, tmdbId] : links) {
		links_file << "Movie ID: " << movieId << " -> TMDB ID: " << tmdbId << "\n";
	}
	cout_debug_file << "\t\t";
	timer.printElapsed(cout_debug_file);
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] printLinks() END\n" << endl;
}

void RecommendationSystem::printGenresFrequency(){
	cout_debug_file << "[RECOMMENDATION SYSTEM] printGenresFrequency() BEGIN" << endl;
	Timer timer("write genres_frequency.txt");
	ofstream genres_file("../out/genres_frequency.txt");
	if (!genres_file) {
		cout_debug_file << "No se pudo abrir el archivo genres_frequency.txt\n";
		return;
	}
	for (const auto& [genre, count] : this->genres_map) {
		genres_file << genre << ": " << count << "\n";
	}
	cout_debug_file << "\t\t";
	timer.printElapsed(cout_debug_file);
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] printGenresFrequency() END\n" << endl;
}


bool RecommendationSystem::userExists(int idUser){
	return this->users.find(idUser) != this->users.end();
}

void RecommendationSystem::recalificarPelicula(int idUser, int idMovie, float rating){
	cout_debug_file << "[RECOMMENDATION SYSTEM] recalificarPelicula() BEGIN" << endl;
	if(users.find(idUser) == users.end()){
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << idUser << " not found." << endl;
		return;
	}
	if(movies.find(idMovie) == movies.end()){
		cout_debug_file << "\t[RECOMMENDATION SYSTEM] Movie ID " << idMovie << " not found." << endl;
		return; // Skip if movie does not exist
	}
	user_movie_ratings[idUser][idMovie] = rating; // Update the rating
	cout_debug_file << "\t[RECOMMENDATION SYSTEM] User " << idUser << " recalified movie ID " << idMovie 
					<< " with rating: " << fixed << setprecision(1) << rating << endl;
	cout_debug_file << "[RECOMMENDATION SYSTEM] recalificarPelicula() END\n" << endl;
	// printUser(); // Print the user ratings after updating
}

vector<tuple<int, string, vector<string>>> RecommendationSystem::getUnratedMoviesByGenre(int userId, const string& genre, int limit){
	vector<tuple<int, string, vector<string>>> resultado;

    if (user_movie_ratings.find(userId) == user_movie_ratings.end()) return resultado;

    const auto& peliculasVistas = user_movie_ratings[userId];

    for (const auto& [movieId, info] : movies) {
        if (peliculasVistas.find(movieId) != peliculasVistas.end()) continue; // ya la calificó

        const auto& [titulo, generos] = info;

        if (find(generos.begin(), generos.end(), genre) != generos.end()) {
            resultado.emplace_back(movieId, titulo, generos);
            if ((int)resultado.size() >= limit) break;
        }
    }

    return resultado;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 											END OTHERS
///////////////////////////////////////////////////////////////////////////////////////////////////////