#include "../header/recommendationSystem.h"
#include "../header/timer.h"

int main(){
    RecommendationSystem sistema;
    std::cout << "termino cons" << std::endl;
    int n = 100;
    // User ID : 214831->7266 ratings
    int user = 214831;
    string metric = "cosine";
    auto vecinos = sistema.knnParalelo(n, user, metric);
    auto recs = sistema.recomendar(vecinos, user);
    auto final = sistema.recomendarMovie(recs, vecinos, user, metric);
    //    std::cout << "1" << std::endl;
    //    while(1){
    // 	;
    //    }
    return 0;
}
