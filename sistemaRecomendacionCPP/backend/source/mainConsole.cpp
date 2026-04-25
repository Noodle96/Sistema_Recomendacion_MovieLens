#include "../header/recommendationSystem.h"
#include "../header/timer.h"

void startRecommendationSystem(){
    RecommendationSystem recommendationSystem;
    cout << "Menu Principal:" << endl;
	cout << "\t0. Salir" << endl;
	cout << "\t1. Calcular distancia euclidiana entre userA and userB" << endl;
	cout << "\t2. Calcular distancia de manhattan entre userA and userB" << endl;
	cout << "\t3. Calculate similitud del coseno entre userA and userB" << endl;
	cout << "\t4. Calcular coeficiente de correlacion de pearson entre userA and userB" << endl;
    cout << "\t5 y 500(kn paralelo). Calcular KNN" << endl;
    cout << "\t6. Recomendar peliculas a un usuario" << endl;
    cout << "\t7. Recomendar movies a un usuario" << endl;
    cout << "\t8.- Agregar un nuevo usuario" << endl;
    cout << "\t9.- Calificar peliculas de un usuario" << endl;

    int choise;
    while(true){
        cout << "Input opcion: ";
        cin>>choise;
        switch (choise){
            case 0:{
                cout << "\tSaliendo del sistema de recomendacion..." << endl;
                exit(0);
                break;
            }
            case 1:{
                /*
                    Calcular distancia euclidiana entre userA and userB
                */
                int userA, userB;
                pair<float, bool> result;
                int commonMovies;
                cout << "\tInsert users (valid ids) case 1" << endl;
                cin>> userA >> userB;
                result = recommendationSystem.calculateEuclideanDistanceDebug(userA, userB, commonMovies);
                cout << "\tCalculating Euclidean distance between user " << userA << " and user " << userB << endl;
                if(result.second) cout << "distance: " << fixed << setprecision(8) << result.first  << " y commonMovies: "<< commonMovies<< endl;
                else cout << "Usuario no valido or no hay peliculas en comun" << endl;
                break;
            }
            case 2:{
                /*
                    Calcular distancia de manhattan entre userA and userB
                */
                int userA, userB;
                pair<float, bool> result;
                int commonMovies;
                cout << "\tInsert users (valid ids) case 2" << endl;
                cin>> userA >> userB;
                cout << "\tCalculating Manhattan distance between user " << userA << " and user " << userB << endl;
                result = recommendationSystem.calculateManhattanDistanceDebug(userA, userB, commonMovies);
                if(result.second) cout << "distance: " << fixed << setprecision(8)<< result.first << " y commonMovies: " << commonMovies<< endl;
                else cout << "Usuario no valido or no hay peliculas en comun" << endl;
                break;
            }
            case 3:{
                /*
                    Calculate similitud del coseno entre userA and userB
                */
                int userA, userB;
                pair<float, bool> result;
                int commonMovies;
                cout << "\tInsert users (valid ids) case 3" << endl;
                cin>> userA >> userB;
                cout << "\tCalculating cosine similarity between user " << userA << " and user " << userB << endl;
                result = recommendationSystem.calculateCosineSimilarityDebug(userA, userB, commonMovies);
                if(result.second) cout << "cosine similarity: "<< fixed << setprecision(8) << result.first << " y commonMovies: " << commonMovies << endl;
                else cout << "Usuario no valido or no hay peliculas en comun" << endl;
                break;
            }
            case 4:{
                /*
                    Calcular coeficiente de correlacion de pearson entre userA and userB
                */
                int userA, userB;
                pair<float, bool> result;
                int commonMovies;
                cout << "\tInsert users (valid ids) case 4" << endl;
                cin>> userA >> userB;
                cout << "\tCalculating Pearson correlation coefficient between user " << userA << " and user " << userB << endl;
                result = recommendationSystem.calculatePearsonCorrelationDebug(userA, userB, commonMovies);
                if(result.second) cout << "pearson correlation: "<< fixed << setprecision(8) << result.first << " y commonMovies: " << commonMovies << endl;
                else cout << "Usuario no valido or no hay peliculas en comun" << endl;
                break;
            }
            case 5:{
                /*
                    Calcular KNN
                */
                int userA;
                int n;
                string metric;
                vector<pair<int, float>> knnResults;
                ofstream& log02 = recommendationSystem.getCoutDebugFile02CalcularKNN();
                log02 << "[KNN] knn(n, user, metrica) BEGIN" << endl;
                cout << "\tInsert user id, n (number of neighbors) and metric (euclidean, manhattan, cosine, pearson): " << endl;
                cin >> userA >> n >> metric;

                cout << "\tCalculating KNN for user " << userA << " with n = " << n << " and metric = " << metric << endl;
                log02 << "\t[KNN] Calculating KNN for user " << userA << " with n = " << n << " and metric = " << metric << endl;
                knnResults.clear();
                Timer timerknn("[KNN main]");
                knnResults = recommendationSystem.knn(n, userA, metric);
                log02 << "\t";
                timerknn.printElapsed(log02, "seg");
                log02 << "\t[KNN] knn(n, user, metrica) END\n" << endl;
                if(knnResults.empty()){
                    cout << "\tNo hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;
                    log02 << "\t[KNN] No hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;

                } else {
                    cout << "\tKNN Results for user " << userA << ":" << endl;
                    log02 << "\tKNN Results for user " << userA << ":" << endl;
                    string str = (metric == "euclidean" || metric == "manhattan") ? "Distance" : (metric == "cosine" ? "Similaridad": "Correlacion");
                    for(const auto& [userId, distance] : knnResults){
                        // cout << "\tUser ID: " << userId << ", Distance: " << distance << endl;
                        log02 << "\t\tUser ID: " << userId << ", " << str <<": "<< fixed << setprecision(8) << distance << endl;
                    }
                }
                log02 << "[KNN] knn(n, user, metrica) END\n\n" << endl;
                break;
            }
            case 500:{
                /*
                    Calcular KNN
                */
                int userA;
                int n;
                string metric;
                vector<pair<int, float>> knnResults;
                ofstream& log02 = recommendationSystem.getCoutDebugFile02CalcularKNN();
                log02 << "[KNN] knn(n, user, metrica) BEGIN" << endl;
                cout << "\tInsert user id, n (number of neighbors) and metric (euclidean, manhattan, cosine, pearson): " << endl;
                cin >> userA >> n >> metric;

                cout << "\tCalculating KNN for user " << userA << " with n = " << n << " and metric = " << metric << endl;
                log02 << "\t[KNN] Calculating KNN for user " << userA << " with n = " << n << " and metric = " << metric << endl;
                knnResults.clear();
                Timer timerknn("[KNN main]");
                knnResults = recommendationSystem.knnParalelo(n, userA, metric);
                log02 << "\t";
                timerknn.printElapsed(log02, "seg");
                log02 << "\t[KNN] knn(n, user, metrica) END\n" << endl;
                if(knnResults.empty()){
                    cout << "\tNo hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;
                    log02 << "\t[KNN] No hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;

                } else {
                    cout << "\tKNN Results for user " << userA << ":" << endl;
                    log02 << "\tKNN Results for user " << userA << ":" << endl;
                    string str = (metric == "euclidean" || metric == "manhattan") ? "Distance" : (metric == "cosine" ? "Similaridad": "Correlacion");
                    for(const auto& [userId, distance] : knnResults){
                        // cout << "\tUser ID: " << userId << ", Distance: " << distance << endl;
                        log02 << "\t\tUser ID: " << userId << ", " << str <<": "<< fixed << setprecision(8) << distance << endl;
                    }
                }
                log02 << "[KNN] knn(n, user, metrica) END\n\n" << endl;
                break;
            }
            case 6:{
                /*
                    Recomendar peliculas a un usuario
                */
                int userA;
                vector<pair<int, float>> knnResults;
                knnResults.clear();
                int n, idUser; string metrica;
                cout << "insertar idUser-n-metrica(euclidean, manhattan, cosine, pearson)" << endl;
                cin >> idUser >> n >> metrica;
                knnResults = recommendationSystem.knnParalelo(n, idUser, metrica);
                if(knnResults.empty()){
                    cout << "\tNo hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;
                    break;
                }
                recommendationSystem.recomendar(knnResults, idUser);
                break;
            }
            case 7: {
                /*
                    Recomendacion con promedio a un usuario X
                */
                int userA;
                vector<pair<int, float>> knnResults;
                knnResults.clear();
                int n, idUser; string metrica;
                cout << "insertar idUser-n-metrica(euclidean, manhattan, cosine, pearson)" << endl;
                cin >> idUser >> n >> metrica;
                ofstream& log4 = recommendationSystem.getCoutDebugFile04PeliculasRecomendar();
                log4 << "\t[KNN main] knn+recomendarMovie begin" << endl;
                Timer timer("\t[KNN main]");
                knnResults = recommendationSystem.knnParalelo(n, idUser, metrica);
                if(knnResults.empty()){
                    cout << "\tNo hay usuarios con peliculas en comun o no hay usuarios registrados.\n\n" << endl;
                    break;
                }
                unordered_map<int, vector<pair<float, int>>> recomendaciones;
                recomendaciones = recommendationSystem.recomendar(knnResults, idUser);
                log4 << "\t";
                timer.printElapsed(log4, "seg");
                log4 << "\t[KNN main] knn+recomendarMovie END" << endl;
                recommendationSystem.recomendarMovie(recomendaciones, idUser);
                cout << "Eso es todo !!!" << endl;
                break;
            }
            case 8:{
                /*
                    Agregar un nuevo usuario
                */
                cout << "\tAgregando un nuevo usuario..." << endl;
                int newUser = recommendationSystem.addUser();
                cout << "\tUsuario agregado exitosamente con el id: " << newUser<< endl;
                break;
            }
            case 9:{
                /*
                    Calificar peliculas de un usuario
                */
                int idUser;
                cout << "\t Inserte el id del Usuario: " << endl;
                cin >> idUser;
                vector<pair<int, float>> peliculas;
                while(1){
                    try{
                        int peliculaId;
                        cout << "\tInserte ID de la pelicula (0 para terminar): ";
                        cin>> peliculaId;
                        if(peliculaId == 0) break; // salir del bucle si se ingresa 0
                        float rating;
                        cout << "\tInserte rating de la pelicula: ";
                        cin>> rating;
                        peliculas.emplace_back(peliculaId, rating);
                    }
                    catch(const std::exception& e){
                        std::cerr << e.what() << " russel say: entrada invalida, ingrese números"<<  '\n';
                    }
                    
                }
                recommendationSystem.calificarPeliculas(idUser, peliculas);
                cout << "\tPeliculas calificadas exitosamente para el usuario " << idUser << "." << endl;
                break;
            }
            default:
                cout << "\tOpción inválida. Intente nuevamente." << endl << endl;
                break;
        } // end switch case
    }
}

int main(){
    // ios_base::sync_with_stdio(false);
    // cin.tie(0);
    // #ifdef DEBUG
    //     // freopen("input.txt","r",stdin);
    //     freopen("output.txt","w",stdout);
    // #endif
    startRecommendationSystem();
    return 0;
}

// g++ -std=c++17 -pthread main.cpp recommendationSystem.cpp -o recommendation_system
// with crow -Iinclude
//g++ -std=c++17 main.cpp recommendationSystem.cpp -o server -lpthread -lboost_system -I../include
