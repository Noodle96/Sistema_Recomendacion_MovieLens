// src/pages/Home.jsx
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import {
  enrichMoviesWithTmdb,
  extractMoviesPayload,
  getMovieId,
  getMovies,
  getRatings,
  getRecs,
  rateMovie
} from "../api/api";
import MovieCard from "../components/MovieCard";

const PAGE_SIZE = 12;
const RECOMMENDATION_LIMIT = 30;

export default function Home() {
  const [ratedMovies, setRatedMovies] = useState([]);
  const [recommendedMovies, setRecommendedMovies] = useState([]);
  const [guestMovies, setGuestMovies] = useState([]);
  const [loading, setLoading] = useState(true);
  const [recommendationsLoading, setRecommendationsLoading] = useState(false);
  const [feedback, setFeedback] = useState("");
  const [ratedPage, setRatedPage] = useState(1);
  const [recommendedPage, setRecommendedPage] = useState(1);
  const [recommendedTotalPages, setRecommendedTotalPages] = useState(1);
  const [recommendedTotal, setRecommendedTotal] = useState(0);
  const [guestPage, setGuestPage] = useState(1);
  const [guestTotalPages, setGuestTotalPages] = useState(1);
  const user = useMemo(() => JSON.parse(localStorage.getItem("user") || "null"), []);
  const navigate = useNavigate();
  const recommendationRequestRef = useRef(0);
  const ratedTotalPages = Math.max(1, Math.ceil(ratedMovies.length / PAGE_SIZE));

  const paginatedRatedMovies = useMemo(() => {
    const start = (ratedPage - 1) * PAGE_SIZE;
    return ratedMovies.slice(start, start + PAGE_SIZE);
  }, [ratedMovies, ratedPage]);

  const groupedRecommendedMovies = useMemo(() => {
    const grouped = recommendedMovies.reduce((acc, movie) => {
      const genre = Array.isArray(movie.genres) && movie.genres.length > 0
        ? movie.genres[0]
        : "Sin genero";

      if (!acc[genre]) {
        acc[genre] = [];
      }
      acc[genre].push(movie);
      return acc;
    }, {});

    return Object.entries(grouped)
      .sort(([a], [b]) => a.localeCompare(b))
      .map(([genre, movies]) => ({ genre, movies }));
  }, [recommendedMovies]);

  const paginatedGuestMovies = useMemo(() => {
    return guestMovies;
  }, [guestMovies]);

  const currentRatedIds = useMemo(
    () => new Set(ratedMovies.map((movie) => getMovieId(movie)).filter(Boolean)),
    [ratedMovies]
  );

  const loadGuestPage = useCallback(async (page) => {
    const payload = await getMovies({ page, limit: PAGE_SIZE });
    const payloadMovies = extractMoviesPayload(payload);
    const enriched = await enrichMoviesWithTmdb(payloadMovies);
    setGuestMovies(enriched);

    const total = Number(payload?.total_pages || payload?.pages || 1);
    setGuestTotalPages(Number.isFinite(total) && total > 0 ? total : 1);
  }, []);

  const loadRecommendationPage = useCallback(async ({ page = 1, ratedIds = new Set() } = {}) => {
    if (!user) return;

    setRecommendationsLoading(true);
    const requestId = ++recommendationRequestRef.current;

    const recsRes = await getRecs(user.id, {
      n: 80,
      metric: "cosine",
      page,
      limit: RECOMMENDATION_LIMIT
    });

    const recsPayload = extractMoviesPayload(recsRes);
    const filteredRecs = recsPayload.filter((movie) => !ratedIds.has(getMovieId(movie)));

    setRecommendedMovies(filteredRecs);
    setRecommendedPage(Number(recsRes?.page || page));
    setRecommendedTotalPages(Number(recsRes?.pages || 1));
    setRecommendedTotal(Number(recsRes?.total || filteredRecs.length));
    setRecommendationsLoading(false);

    enrichMoviesWithTmdb(filteredRecs)
      .then((enrichedRecs) => {
        if (recommendationRequestRef.current === requestId) {
          setRecommendedMovies(enrichedRecs);
        }
      })
      .catch(() => {
        // Keep fast non-enriched payload if TMDB enrichment fails.
      });
  }, [user]);

  const loadUserSections = useCallback(async () => {
    if (!user) return;

    const ratingsRes = await getRatings(user.id);

    const ratingsPayload = Array.isArray(ratingsRes?.ratings) ? ratingsRes.ratings : [];
    const ratedIds = new Set(ratingsPayload.map((movie) => getMovieId(movie)).filter(Boolean));
    const enrichedRated = await enrichMoviesWithTmdb(ratingsPayload);

    setRatedMovies(enrichedRated);
    setRatedPage(1);

    await loadRecommendationPage({ page: 1, ratedIds });
  }, [user, loadRecommendationPage]);

  useEffect(() => {
    const load = async () => {
      setLoading(true);
      if (user) {
        await loadUserSections();
      } else {
        await loadGuestPage(1);
        setGuestPage(1);
      }
      setLoading(false);
    };
    load();
  }, [user, loadUserSections, loadGuestPage]);

  const handleRate = async (movieId, rating) => {
    if (!user) {
      setFeedback("Inicia sesion para calificar peliculas.");
      return;
    }

    await rateMovie(user.id, movieId, rating);
    await loadUserSections();
    setFeedback("Calificacion guardada. Actualizamos tus recomendaciones.");
    setTimeout(() => setFeedback(""), 2500);
  };

  const onGuestPageChange = async (nextPage) => {
    if (nextPage < 1 || nextPage > guestTotalPages) return;
    setGuestPage(nextPage);

    setLoading(true);
    await loadGuestPage(nextPage);
    setLoading(false);
  };

  const welcomeTitle = user ? `Hola, ${user.username || "cinefilo"}` : "Bienvenido";

  return (
    <main className="nf-page">
      <section className="nf-hero">
        <div className="nf-hero-content">
          <p className="nf-kicker">Streaming personalizado</p>
          <h1>{welcomeTitle}</h1>
          <p>
            Descubre peliculas populares y recomendaciones creadas con tu historial.
            Tu proxima maraton empieza aqui.
          </p>
          <div className="nf-hero-actions">
            <Link className="nf-btn nf-btn-primary" to="/explore">
              Explorar catalogo
            </Link>
            <Link className="nf-btn nf-btn-ghost" to="/login">
              {user ? "Cambiar cuenta" : "Iniciar sesion"}
            </Link>
          </div>
          {feedback && <span className="nf-feedback">{feedback}</span>}
        </div>
      </section>

      <section className="nf-section">
        <div className="nf-section-header">
          <h2>{user ? "Peliculas que has puntuado" : "Tendencias"}</h2>
          <span>{user ? ratedMovies.length : guestMovies.length} titulos</span>
        </div>

        {loading ? (
          <div className="nf-loading-grid">
            {Array.from({ length: 8 }).map((_, i) => (
              <div key={i} className="nf-skeleton" />
            ))}
          </div>
        ) : (
          <div className="nf-grid">
            {(user ? paginatedRatedMovies : paginatedGuestMovies).map((movie) => (
              <MovieCard
                key={getMovieId(movie)}
                movie={movie}
                onClick={() => navigate(`/movie/${movie.tmdb_id || getMovieId(movie)}`, { state: { movie } })}
                onRate={(rating) => handleRate(getMovieId(movie), rating)}
                showRateAction={Boolean(user)}
              />
            ))}
          </div>
        )}

        {!loading && !user && guestTotalPages > 1 && (
          <div className="nf-pagination">
            <button onClick={() => onGuestPageChange(guestPage - 1)} disabled={guestPage === 1}>
              Anterior
            </button>
            <span>
              Pagina {guestPage} de {guestTotalPages}
            </span>
            <button
              onClick={() => onGuestPageChange(guestPage + 1)}
              disabled={guestPage === guestTotalPages}
            >
              Siguiente
            </button>
          </div>
        )}

        {!loading && user && ratedTotalPages > 1 && (
          <div className="nf-pagination">
            <button onClick={() => setRatedPage((p) => Math.max(1, p - 1))} disabled={ratedPage === 1}>
              Anterior
            </button>
            <span>
              Pagina {ratedPage} de {ratedTotalPages}
            </span>
            <button
              onClick={() => setRatedPage((p) => Math.min(ratedTotalPages, p + 1))}
              disabled={ratedPage === ratedTotalPages}
            >
              Siguiente
            </button>
          </div>
        )}
      </section>

      {user && (
        <section className="nf-section">
          <div className="nf-section-header">
            <h2>Peliculas recomendadas para ti</h2>
            <span>{recommendedTotal || recommendedMovies.length} titulos</span>
          </div>

          <div className="nf-recs-actions">
            <button
              className="nf-btn nf-btn-ghost"
              onClick={async () => {
                await loadRecommendationPage({ page: 1, ratedIds: currentRatedIds });
              }}
            >
              Actualizar recomendaciones
            </button>
          </div>

          {recommendationsLoading ? (
            <div className="nf-loading-grid">
              {Array.from({ length: 8 }).map((_, i) => (
                <div key={i} className="nf-skeleton" />
              ))}
            </div>
          ) : (
            <div className="nf-genre-groups">
              {groupedRecommendedMovies.map(({ genre, movies }) => (
                <div key={genre} className="nf-genre-group">
                  <div className="nf-genre-title-row">
                    <h3 className="nf-genre-title">{genre}</h3>
                    <span>{movies.length} titulos</span>
                  </div>
                  <div className="nf-grid">
                    {movies.map((movie) => (
                      <MovieCard
                        key={getMovieId(movie)}
                        movie={movie}
                        onClick={() =>
                          navigate(`/movie/${movie.tmdb_id || getMovieId(movie)}`, { state: { movie } })
                        }
                        showBackendRating={false}
                        showRateAction={false}
                      />
                    ))}
                  </div>
                </div>
              ))}
            </div>
          )}

          {!recommendationsLoading && recommendedTotalPages > 1 && (
            <div className="nf-pagination">
              <button
                onClick={() =>
                  loadRecommendationPage({
                    page: Math.max(1, recommendedPage - 1),
                    ratedIds: currentRatedIds
                  })
                }
                disabled={recommendedPage === 1}
              >
                Anterior
              </button>
              <span>
                Pagina {recommendedPage} de {recommendedTotalPages}
              </span>
              <button
                onClick={() =>
                  loadRecommendationPage({
                    page: Math.min(recommendedTotalPages, recommendedPage + 1),
                    ratedIds: currentRatedIds
                  })
                }
                disabled={recommendedPage === recommendedTotalPages}
              >
                Siguiente
              </button>
            </div>
          )}
        </section>
      )}

      {user && !loading && ratedMovies.length === 0 && (
        <section className="nf-section">
          <p className="nf-empty">
            Aun no tienes peliculas puntuadas. Puedes empezar puntuando desde Explorar.
          </p>
        </section>
      )}

      {user && !loading && recommendedMovies.length === 0 && (
        <section className="nf-section">
          <p className="nf-empty">
            Aun no hay recomendaciones disponibles. Puntua mas peliculas para mejorar tus gustos.
          </p>
        </section>
      )}
    </main>
  );
}