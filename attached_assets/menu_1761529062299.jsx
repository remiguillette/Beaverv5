function resolveIcon(iconName) {
  const lucideReact = window.LucideReact;
  return lucideReact && lucideReact.icons ? lucideReact.icons[iconName] : undefined;
}

const AppTile = ({ name, icon: Icon, accent, onClick }) => {
  const fallbackInitial = name.charAt(0).toUpperCase();

  return (
    <button type="button" className={`app-tile app-tile--${accent}`} onClick={onClick}>
      <div className="app-tile__icon" aria-hidden="true">
        {Icon ? <Icon size={42} strokeWidth={1.75} /> : <span>{fallbackInitial}</span>}
      </div>
      <h3 className="app-tile__name">{name}</h3>
    </button>
  );
};

const VALID_ACCENTS = new Set(['amber', 'violet', 'cyan', 'red', 'green']);

function resolveAccent(value) {
  if (typeof value === 'string' && VALID_ACCENTS.has(value)) {
    return value;
  }

  return 'amber';
}

function mapRemoteApps(remote) {
  if (!remote || !Array.isArray(remote.apps)) {
    return [];
  }

  return remote.apps
    .map((app) => {
      const name = typeof app?.name === 'string' ? app.name.trim() : '';

      if (!name) {
        return null;
      }

      const iconName = typeof app.icon === 'string' ? app.icon : undefined;

      return {
        name,
        accent: resolveAccent(app.accent),
        icon: iconName ? resolveIcon(iconName) : undefined,
      };
    })
    .filter(Boolean);
}

function resolveBackendBaseUrl() {
  if (typeof window === 'undefined' || typeof window.location === 'undefined') {
    return 'http://127.0.0.1:5000';
  }

  const { protocol, host } = window.location;

  if (protocol === 'http:' || protocol === 'https:') {
    return `${protocol}//${host}`;
  }

  return 'http://127.0.0.1:5000';
}

const BeaverMenu = () => {
  const [apps, setApps] = React.useState([]);
  const [error, setError] = React.useState(null);
  const [isLoading, setIsLoading] = React.useState(true);

  React.useEffect(() => {
    let cancelled = false;

    async function loadMenu() {
      try {
        setError(null);
        setIsLoading(true);
        const baseUrl = resolveBackendBaseUrl();
        const response = await fetch(`${baseUrl}/api/menu`, {
          headers: {
            Accept: 'application/json',
          },
        });

        if (!response.ok) {
          throw new Error(`Backend responded with ${response.status}`);
        }

        const payload = await response.json();
        if (!cancelled) {
          const mapped = mapRemoteApps(payload);
          setApps(mapped);
          setError(mapped.length === 0 ? 'No applications available.' : null);
          setIsLoading(false);
        }
      } catch (fetchError) {
        if (!cancelled) {
          console.error('Failed to load menu data', fetchError);
          setError('Unable to reach the BeaverKiosk backend (port 5000).');
          setIsLoading(false);
        }
      }
    }

    loadMenu();

    return () => {
      cancelled = true;
    };
  }, []);

  return (
    <div className="menu-root">
      <header className="menu-header">
        <h1>BeaverKiosk</h1>
        <p className="subtitle">Unified application launcher</p>
      </header>

      <main className="menu-grid">
        {apps.map((app) => (
          <AppTile key={app.name} {...app} />
        ))}
        {apps.length === 0 && isLoading && (
          <div className="menu-status" role="status">
            <p>Loading menu from Node backend&hellip;</p>
          </div>
        )}
        {apps.length === 0 && !isLoading && error && (
          <div className="menu-status" role="status">
            <p>{error}</p>
            <p className="menu-status__hint">
              Ensure the Node.js backend is running on port 5000.
            </p>
          </div>
        )}
      </main>

      <footer className="menu-footer">
        <small>Electron • React • Beaver Suite ©2025</small>
      </footer>
    </div>
  );
};

const rootElement = document.getElementById('root');

if (rootElement) {
  const root = ReactDOM.createRoot(rootElement);
  root.render(<BeaverMenu />);
}
