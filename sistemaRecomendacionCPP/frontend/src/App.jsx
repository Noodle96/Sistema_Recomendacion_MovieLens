import { BrowserRouter, NavLink, Routes, Route } from "react-router-dom";
import Login from "./pages/Login";
import Home from "./pages/Home";
import Explore from "./pages/Explore";
import MovieDetail from "./pages/MovieDetail";
import Register from "./pages/Register";

export default function App() {
  return (
    <BrowserRouter>
      <div className="nf-app-shell">
        <header className="nf-nav">
          <div className="nf-brand">MOVIELENS</div>
          <nav className="nf-links">
            <NavLink to="/" end>
              Inicio
            </NavLink>
            <NavLink to="/explore">Explorar</NavLink>
            <NavLink to="/login">Login</NavLink>
            <NavLink to="/register">Registro</NavLink>
          </nav>
        </header>
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/login" element={<Login />} />
          <Route path="/register" element={<Register />} />
          <Route path="/explore" element={<Explore />} />
          <Route path="/movie/:movieId" element={<MovieDetail />} />
        </Routes>
      </div>
    </BrowserRouter>
  );
}
