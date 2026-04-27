// src/pages/Login.jsx
import { useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import { login } from "../api/api";

export default function Login() {
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();

  const handleLogin = async () => {
    try {
      setLoading(true);
      setError("");
      const res = await login(username, password);
      if (res.id) {
        localStorage.setItem("user", JSON.stringify(res));
        navigate("/");
      } else {
        setError("Credenciales invalidas. Intentalo de nuevo.");
      }
    } catch (e) {
      setError(e.message || "No se pudo conectar con el servidor.");
    } finally {
      setLoading(false);
    }
  };

  return (
    <main className="nf-page nf-login-page">
      <section className="nf-login-card">
        <h1>Iniciar sesion</h1>
        <p>Accede para recibir recomendaciones personalizadas.</p>

        <label>Usuario</label>
        <input
          value={username}
          onChange={(e) => setUsername(e.target.value)}
          placeholder="Tu usuario"
        />

        <label>Contrasena</label>
        <input
          value={password}
          type="password"
          onChange={(e) => setPassword(e.target.value)}
          placeholder="********"
        />

        {error && <p className="nf-error">{error}</p>}

        <button className="nf-btn nf-btn-primary nf-login-button" onClick={handleLogin}>
          {loading ? "Entrando..." : "Entrar"}
        </button>

        <Link className="nf-btn nf-btn-ghost" to="/register">
          Crear cuenta nueva
        </Link>
      </section>
    </main>
  );
}