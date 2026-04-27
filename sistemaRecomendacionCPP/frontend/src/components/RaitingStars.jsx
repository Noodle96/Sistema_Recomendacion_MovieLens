// src/components/RatingStars.jsx
export default function RatingStars({
  value = 0,
  max = 5,
  interactive = false,
  onRate,
  label
}) {
  const safeValue = Number(value) || 0;

  return (
    <div className="rating-stars-wrap" onClick={(e) => e.stopPropagation()}>
      <div className="rating-stars">
        {Array.from({ length: max }).map((_, idx) => {
          const n = idx + 1;
          const isActive = safeValue >= n - 0.25;

          if (interactive) {
            return (
              <button
                key={n}
                type="button"
                className={isActive ? "active" : ""}
                onClick={() => onRate?.(n)}
                aria-label={`Calificar con ${n} estrellas`}
              >
                ★
              </button>
            );
          }

          return (
            <span key={n} className={isActive ? "active" : ""} aria-hidden="true">
              ★
            </span>
          );
        })}
      </div>
      {label && <small>{label}</small>}
    </div>
  );
}