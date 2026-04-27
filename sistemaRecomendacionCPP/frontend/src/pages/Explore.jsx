// src/pages/Explore.jsx
import { useMemo, useState } from "react";
import { useNavigate } from "react-router-dom";
import { enrichMoviesWithTmdb, extractMoviesPayload, getMovieId, searchMovies } from "../api/api";
import MovieCard from "../components/MovieCard";

const PAGE_SIZE = 12;

export default function Explore() {
  const [q, setQ] = useState("");
  const [movies, setMovies] = useState([]);
  const [loading, setLoading] = useState(false);
  const [currentPage, setCurrentPage] = useState(1);
  const navigate = useNavigate();
  const totalPages = Math.max(1, Math.ceil(movies.length / PAGE_SIZE));

  const paginatedMovies = useMemo(() => {
    const start = (currentPage - 1) * PAGE_SIZE;
    return movies.slice(start, start + PAGE_SIZE);
  }, [movies, currentPage]);

  const handleSearch = async () => {
    if (!q.trim()) return;
    setLoading(true);
    const res = await searchMovies(q);
    const payloadMovies = extractMoviesPayload(res);
    const enriched = await enrichMoviesWithTmdb(payloadMovies);
    setMovies(enriched);
    setCurrentPage(1);
    setLoading(false);
  };

  const handleKeyDown = (e) => {
    if (e.key === "Enter") {
      handleSearch();
    }
  };

  return (
    <main className="nf-page">
      <section className="nf-explore-head">
        <h1>Explorar catalogo</h1>
        <p>Busca por titulo y encuentra algo para esta noche.</p>
        <div className="nf-search-wrap">
          <input
            value={q}
            onChange={(e) => setQ(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="Buscar pelicula..."
          />
          <button className="nf-btn nf-btn-primary" onClick={handleSearch}>
            Buscar
          </button>
        </div>
      </section>

      <section className="nf-section">
        {loading ? (
          <div className="nf-loading-grid">
            {Array.from({ length: 10 }).map((_, i) => (
              <div key={i} className="nf-skeleton" />
            ))}
          </div>
        ) : (
          <div className="nf-grid">
            {paginatedMovies.map((movie) => (
              <MovieCard
                key={getMovieId(movie)}
                movie={movie}
                onClick={() => navigate(`/movie/${movie.tmdb_id || getMovieId(movie)}`, { state: { movie } })}
              />
            ))}
          </div>
        )}

        {!loading && movies.length === 0 && (
          <p className="nf-empty">No hay resultados aun. Prueba con otro titulo.</p>
        )}

        {!loading && movies.length > 0 && totalPages > 1 && (
          <div className="nf-pagination">
            <button onClick={() => setCurrentPage((p) => Math.max(1, p - 1))} disabled={currentPage === 1}>
              Anterior
            </button>
            <span>
              Pagina {currentPage} de {totalPages}
            </span>
            <button
              onClick={() => setCurrentPage((p) => Math.min(totalPages, p + 1))}
              disabled={currentPage === totalPages}
            >
              Siguiente
            </button>
          </div>
        )}
      </section>
    </main>
  );
}