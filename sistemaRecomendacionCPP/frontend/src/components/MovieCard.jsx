// src/components/MovieCard.jsx
import RatingStars from "./RaitingStars";

function resolvePoster(movie) {
  const candidates = [
    movie.poster_path,
    movie.poster,
    movie.poster_url,
    movie.image,
    movie.image_url
  ].filter(Boolean);

  if (candidates.length > 0) {
    const poster = String(candidates[0]);
    return poster.startsWith("http")
      ? poster
      : `https://image.tmdb.org/t/p/w500${poster}`;
  }

  return "https://placehold.co/300x450/141414/e50914?text=No+Poster";
}

export default function MovieCard({
  movie,
  onClick,
  onRate,
  showRateAction = false,
  showBackendRating = true
}) {
  const poster = resolvePoster(movie);
  const backendRating = Number(
    movie.backend_rating ?? movie.predicted_rating ?? movie.score ?? movie.rating ?? 0
  );
  const year = String(movie.release_date || movie.year || "").slice(0, 4);
  const overview = movie.overview || movie.description || "";
  const backendStars = Math.max(0, Math.min(5, backendRating));
  const hasBackendStars = backendStars > 0;

  return (
    <article className="movie-card" onClick={onClick}>
      <img src={poster} alt={movie.title} loading="lazy" decoding="async" />
      <div className="movie-overlay">
        <h3>{movie.title}</h3>
        <div className="movie-meta">
          {year && <span>{year}</span>}
          {showBackendRating && hasBackendStars && <span>Backend {backendStars.toFixed(1)} / 5</span>}
        </div>
        {showBackendRating && (
          <RatingStars
            value={backendStars}
            label={hasBackendStars ? "Estrellas desde backend" : "Sin estrellas backend"}
          />
        )}
        {overview && <p className="movie-overview">{overview}</p>}
        {showRateAction && (
          <RatingStars
            interactive
            value={backendStars}
            onRate={onRate}
            label="Tu calificacion"
          />
        )}
      </div>
    </article>
  );
}