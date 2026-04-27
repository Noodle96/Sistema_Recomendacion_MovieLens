import { useEffect, useMemo, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import RatingStars from "../components/RaitingStars";
import {
  enrichMoviesWithTmdb,
  extractMoviesPayload,
  getMovieId,
  getPopular,
  rateMovie,
  register
} from "../api/api";

function resolvePoster(movie) {
  const candidate = movie?.poster_path || movie?.poster || movie?.poster_url;
  if (!candidate) return "https://placehold.co/300x450/141414/e50914?text=No+Poster";
  return String(candidate).startsWith("http")
    ? candidate
    : `https://image.tmdb.org/t/p/w500${candidate}`;
}

export default function Register() {
  const [step, setStep] = useState(1);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [createdUser, setCreatedUser] = useState(null);
  const [popularMovies, setPopularMovies] = useState([]);
  const [ratings, setRatings] = useState({});
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const navigate = useNavigate();

  const selectedCount = useMemo(
    () => Object.values(ratings).filter((v) => Number(v) >= 0.5).length,
    [ratings]
  );

  useEffect(() => {
    const loadPopularMovies = async () => {
      if (step !== 2) return;
      try {
        setLoading(true);
        setError("");
        const payload = await getPopular({ limit: 24 });
        const top = extractMoviesPayload(payload);
        const enriched = await enrichMoviesWithTmdb(top);
        setPopularMovies(enriched);
      } catch (e) {
        setError(e.message || "No se pudieron cargar peliculas populares.");
      } finally {
        setLoading(false);
      }
    };

    loadPopularMovies();
  }, [step]);

  const handleRegister = async () => {
    setError("");
    if (!username.trim() || !password.trim()) {
      setError("Completa usuario y contrasena.");
      return;
    }

    try {
      setLoading(true);
      const res = await register(username.trim(), password);
      if (res.id) {
        const user = { id: res.id, username: res.username || username.trim() };
        setCreatedUser(user);
        localStorage.setItem("user", JSON.stringify(user));
        setStep(2);
      } else {
        setError(res.error || "No se pudo registrar el usuario.");
      }
    } catch (e) {
      setError(e.message || "No se pudo conectar con el servidor de registro.");
    } finally {
      setLoading(false);
    }
  };

  const handleRate = (movieId, value) => {
    setRatings((prev) => {
      const current = Number(prev[movieId] || 0);
      if (current === Number(value)) {
        const next = { ...prev };
        delete next[movieId];
        return next;
      }
      return { ...prev, [movieId]: value };
    });
  };

  const saveOnboardingRatings = async () => {
    if (!createdUser) return;

    setSaving(true);
    setError("");

    const entries = Object.entries(ratings)
      .map(([movieId, rating]) => ({ movieId: Number(movieId), rating: Number(rating) }))
      .filter((item) => Number.isFinite(item.movieId) && item.rating >= 0.5);

    try {
      for (const item of entries) {
        await rateMovie(createdUser.id, item.movieId, item.rating);
      }

      navigate("/");
    } catch (e) {
      setError(e.message || "No se pudieron guardar tus preferencias.");
    } finally {
      setSaving(false);
    }
  };

  if (step === 1) {
    return (
      <main className="nf-page nf-login-page">
        <section className="nf-login-card">
          <h1>Crear cuenta</h1>
          <p>Registra tu usuario para empezar a personalizar recomendaciones.</p>

          <label>Usuario</label>
          <input
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            placeholder="Elige un usuario"
          />

          <label>Contrasena</label>
          <input
            value={password}
            type="password"
            onChange={(e) => setPassword(e.target.value)}
            placeholder="********"
          />

          {error && <p className="nf-error">{error}</p>}

          <button className="nf-btn nf-btn-primary nf-login-button" onClick={handleRegister}>
            {loading ? "Creando cuenta..." : "Registrarme"}
          </button>

          <Link className="nf-btn nf-btn-ghost" to="/login">
            Ya tengo cuenta
          </Link>
        </section>
      </main>
    );
  }

  return (
    <main className="nf-page">
      <section className="nf-onboarding-head">
        <p className="nf-kicker">Paso 2 de 2</p>
        <h1>Elige tus favoritas</h1>
        <p>
          Puedes calificar peliculas top para mejorar recomendaciones (opcional). Seleccionadas: {selectedCount}
        </p>
      </section>

      {loading ? (
        <div className="nf-loading-grid">
          {Array.from({ length: 12 }).map((_, i) => (
            <div key={i} className="nf-skeleton" />
          ))}
        </div>
      ) : (
        <div className="nf-grid">
          {popularMovies.map((movie) => {
            const movieId = getMovieId(movie);
            if (!movieId) return null;
            const currentRating = Number(ratings[movieId] || 0);
            const isSelected = currentRating >= 0.5;

            return (
              <article
                key={movieId}
                className={`nf-onboarding-card ${isSelected ? "selected" : ""}`}
              >
                <img src={resolvePoster(movie)} alt={movie.title} loading="lazy" decoding="async" />
                <div className="nf-onboarding-content">
                  <h3>{movie.title}</h3>
                  <p>{isSelected ? "Calificada" : "Opcional: elige estrellas"}</p>
                  <RatingStars
                    interactive
                    value={currentRating}
                    onRate={(value) => handleRate(movieId, value)}
                    label={currentRating ? `${currentRating.toFixed(1)} / 5` : "Sin calificar"}
                  />
                </div>
              </article>
            );
          })}
        </div>
      )}

      <div className="nf-onboarding-actions">
        {error && <p className="nf-error">{error}</p>}
        <button className="nf-btn nf-btn-primary" onClick={saveOnboardingRatings} disabled={saving}>
          {saving ? "Guardando preferencias..." : "Continuar"}
        </button>
      </div>
    </main>
  );
}
