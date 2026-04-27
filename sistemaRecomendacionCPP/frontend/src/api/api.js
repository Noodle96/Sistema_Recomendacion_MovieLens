// src/api/api.js
const API_BASES = [
  "http://localhost:8080/api"
].filter(Boolean);
const TMDB_API = "https://api.themoviedb.org/3";
const TMDB_API_KEY = "a46516e2fa77205885e1f216300b3a97";
const tmdbCache = new Map();

async function fetchApi(path, options = {}) {
  let lastError = null;

  for (const base of API_BASES) {
    try {
      const res = await fetch(`${base}${path}`, options);
      if (!res.ok) {
        const body = await res.json().catch(() => ({}));
        throw new Error(body.error || `Error HTTP ${res.status}`);
      }
      return res;
    } catch (error) {
      lastError = error;
    }
  }

  throw lastError || new Error("No se pudo conectar con el backend");
}

async function fetchApiJson(path, options = {}) {
  const res = await fetchApi(path, options);
  return res.json();
}

export const getMovieId = (movie) => movie.movie_id ?? movie.movieId ?? movie.id;

export const extractMoviesPayload = (payload) => {
  if (!payload) return [];
  if (Array.isArray(payload)) return payload;
  if (Array.isArray(payload.movies)) return payload.movies;
  if (Array.isArray(payload.results)) return payload.results;
  if (Array.isArray(payload.data)) return payload.data;
  return [];
};

export const login = async (username, password) => {
  return fetchApiJson(`/auth/login`, {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({ username, password })
  });
};

export const register = async (username, password) => {
  return fetchApiJson(`/auth/register`, {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({ username, password })
  });
};

export const getPopular = async ({ limit = 20 } = {}) => {
  return fetchApiJson(`/popular?limit=${limit}`);
};

export const getMovies = async ({ page = 1, limit = 20 } = {}) => {
  return fetchApiJson(`/movies?page=${page}&limit=${limit}`);
};

export const searchMovies = async (q) => {
  return fetchApiJson(`/movies/search?q=${encodeURIComponent(q)}`);
};

export const getRecs = async (
  userId,
  { n = 60, metric = "cosine", page = 1, limit = 30 } = {}
) => {
  return fetchApiJson(
    `/recomendar?user_id=${userId}&n=${n}&metric=${metric}&page=${page}&limit=${limit}`
  );
};

export const getRatings = async (userId) => {
  return fetchApiJson(`/ratings?user_id=${userId}`);
};

export const rateMovie = async (user_id, movie_id, rating) => {
  return fetchApi(`/calificar`, {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({ user_id, movie_id, rating })
  });
};

export const saveRecommendation = async ({ userId, movieId, rating = 5 }) => {
  const body = JSON.stringify({ user_id: userId, movie_id: movieId, rating });

  try {
    const recRes = await fetchApi(`/recomendar`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body
    });

    return recRes.json().catch(() => ({ ok: true }));
  } catch {
    // Fallback below keeps compatibility when backend only supports /calificar.
  }

  const fallback = await rateMovie(userId, movieId, rating);
  return fallback.json().catch(() => ({ ok: true }));
};

export const getTmdbMovieById = async (tmdbId) => {
  if (!tmdbId || Number(tmdbId) <= 0) {
    return null;
  }

  if (tmdbCache.has(tmdbId)) {
    return tmdbCache.get(tmdbId);
  }

  try {
    const res = await fetch(
      `${TMDB_API}/movie/${tmdbId}?api_key=${TMDB_API_KEY}&language=es-ES`
    );

    if (!res.ok) {
      return null;
    }

    const data = await res.json();
    tmdbCache.set(tmdbId, data);
    return data;
  } catch {
    return null;
  }
};

export const enrichMoviesWithTmdb = async (movies = []) => {
  const enriched = await Promise.all(
    movies.map(async (movie) => {
      const tmdbId = movie.tmdb_id || movie.tmdbId;
      const tmdbData = await getTmdbMovieById(tmdbId);

      if (!tmdbData) {
        return movie;
      }

      return {
        ...movie,
        title: movie.title || tmdbData.title,
        overview: movie.overview || tmdbData.overview,
        description: movie.description || tmdbData.overview,
        poster_path: movie.poster_path || tmdbData.poster_path,
        backdrop_path: movie.backdrop_path || tmdbData.backdrop_path,
        release_date: movie.release_date || tmdbData.release_date,
        tmdb_vote_average: tmdbData.vote_average
      };
    })
  );

  return enriched;
};
