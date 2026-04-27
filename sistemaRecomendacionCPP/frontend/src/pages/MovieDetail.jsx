import { useEffect, useMemo, useState } from "react";
import { Link, useLocation, useNavigate, useParams } from "react-router-dom";
import RatingStars from "../components/RaitingStars";
import { getMovieId, getTmdbMovieById, saveRecommendation } from "../api/api";

function getPoster(movie) {
  const candidate = movie?.poster_path || movie?.poster || movie?.poster_url;
  if (!candidate) return "https://placehold.co/600x900/141414/e50914?text=No+Poster";
  return String(candidate).startsWith("http")
    ? candidate
    : `https://image.tmdb.org/t/p/w500${candidate}`;
}



export default function MovieDetail() {
  const { movieId } = useParams();
  const location = useLocation();
  const navigate = useNavigate();

  const [movie, setMovie] = useState(location.state?.movie || null);
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [message, setMessage] = useState("");
  const [rating, setRating] = useState(5);

  const user = useMemo(() => JSON.parse(localStorage.getItem("user") || "null"), []);

  useEffect(() => {
    const loadFallback = async () => {
      if (movie) return;
      const tmdbId = Number(movieId);
      if (!Number.isFinite(tmdbId) || tmdbId <= 0) return;

      setLoading(true);
      const tmdb = await getTmdbMovieById(tmdbId);
      if (tmdb) {
        setMovie({
          id: tmdb.id,
          tmdb_id: tmdb.id,
          title: tmdb.title,
          overview: tmdb.overview,
          release_date: tmdb.release_date,
          poster_path: tmdb.poster_path,
          backdrop_path: tmdb.backdrop_path,
          tmdb_vote_average: tmdb.vote_average
        });
      }
      setLoading(false);
    };

    loadFallback();
  }, [movieId, movie]);

  const backendRating = Number(
    movie?.backend_rating ?? movie?.predicted_rating ?? movie?.score ?? movie?.rating ?? 0
  );

  const handleSaveRecommendation = async () => {
    if (!user) {
      setMessage("Debes iniciar sesion para guardar tu recomendacion.");
      return;
    }

    const id = getMovieId(movie) || Number(movieId);
    if (!id) {
      setMessage("No se pudo identificar la pelicula.");
      return;
    }

    try {
      setSaving(true);
      setMessage("");
      await saveRecommendation({ userId: user.id, movieId: id, rating });

      const key = `saved_recommendations_${user.id}`;
      const saved = JSON.parse(localStorage.getItem(key) || "[]");
      const next = Array.from(new Set([...saved, id]));
      localStorage.setItem(key, JSON.stringify(next));

      setMessage("Recomendacion guardada correctamente.");
    } catch {
      setMessage("No se pudo guardar la recomendacion en este momento.");
    } finally {
      setSaving(false);
    }
  };

  if (loading) {
    return (
      <main className="nf-page">
        <p className="nf-empty">Cargando informacion de la pelicula...</p>
      </main>
    );
  }

  if (!movie) {
    return (
      <main className="nf-page">
        <p className="nf-empty">No hay datos de esta pelicula para mostrar.</p>
        <button className="nf-btn nf-btn-ghost" onClick={() => navigate(-1)}>
          Volver
        </button>
      </main>
    );
  }

  return (
    <main className="nf-page">
      <section
        className="nf-detail-hero"
        style={{
          backgroundImage: movie.backdrop_path
            ? `linear-gradient(180deg, rgba(0,0,0,.2), rgba(0,0,0,.85)), url(https://image.tmdb.org/t/p/w1280${movie.backdrop_path})`
            : undefined
        }}
      >
        <img src={getPoster(movie)} alt={movie.title} className="nf-detail-poster" />

        <div className="nf-detail-content">
          <p className="nf-kicker">Detalle de pelicula</p>
          <h1>{movie.title}</h1>
          <p className="nf-detail-text">{movie.overview || movie.description || "Sin descripcion disponible."}</p>

          <div className="nf-detail-meta">
            {movie.release_date && <span>{String(movie.release_date).slice(0, 4)}</span>}
            {backendRating > 0 && <span>Backend {backendRating.toFixed(1)} / 5</span>}
          </div>

          <RatingStars
            value={Math.max(0, Math.min(5, backendRating))}
            label="Estrellas desde backend"
          />

          <div className="nf-detail-save">
            <label>Tu calificacion para guardar recomendacion</label>
            <RatingStars interactive value={rating} onRate={setRating} />

            <button className="nf-btn nf-btn-primary" onClick={handleSaveRecommendation} disabled={saving}>
              {saving ? "Guardando..." : "Guardar recomendacion"}
            </button>
            {message && <p className="nf-feedback">{message}</p>}
          </div>

          <div className="nf-hero-actions">
            <Link className="nf-btn nf-btn-ghost" to="/">
              Volver al inicio
            </Link>
            <Link className="nf-btn nf-btn-ghost" to="/explore">
              Seguir explorando
            </Link>
          </div>
        </div>
      </section>
    </main>
  );
}
