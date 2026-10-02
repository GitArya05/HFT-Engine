import { useState, useEffect, useRef } from 'react';
import { Activity, Zap, Server, ShieldAlert, Wifi, WifiOff } from 'lucide-react';

interface TelemetryData {
  throughput_mps: number;
  best_bid: number;
  best_ask: number;
  total_orders: number;
}

export default function App() {
  const [data, setData] = useState<TelemetryData>({
    throughput_mps: 0,
    best_bid: 0,
    best_ask: 0,
    total_orders: 0
  });
  const [connected, setConnected] = useState<boolean>(false);
  const [lastHeartbeat, setLastHeartbeat] = useState<string>('N/A');
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    const ws = new WebSocket('ws://localhost:8084');
    wsRef.current = ws;

    ws.onopen = () => {
      setConnected(true);
    };

    ws.onmessage = (event) => {
      try {
        const parsedData: TelemetryData = JSON.parse(event.data);
        setData(parsedData);
        setLastHeartbeat(
          new Date().toLocaleTimeString([], {
            hour12: false,
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit',
            fractionalSecondDigits: 3
          })
        );
      } catch (e) {
        console.error('Invalid JSON payload received:', e);
      }
    };

    ws.onclose = () => setConnected(false);
    ws.onerror = () => setConnected(false);

    return () => {
      ws.close();
    };
  }, []);

  const handleKillSwitch = () => {
    if (wsRef.current && connected) {
      wsRef.current.send(JSON.stringify({ command: 'HALT_ENGINE' }));
      alert('Emergency HALT command dispatched to C++ core.');
    }
  };

  // Real derived market statistics
  const bidPrice = data.best_bid > 0 ? (data.best_bid / 100).toFixed(2) : '--.--';
  const askPrice = data.best_ask > 0 ? (data.best_ask / 100).toFixed(2) : '--.--';
  const rawSpread = data.best_ask > 0 && data.best_bid > 0 ? data.best_ask - data.best_bid : 0;
  const spreadDollars = (rawSpread / 100).toFixed(2);
  const spreadPercent = data.best_bid > 0 ? ((rawSpread / data.best_bid) * 100).toFixed(4) : '0.0000';

  return (
    <div style={styles.container}>
      {/* HEADER */}
      <header style={styles.header}>
        <div style={styles.brandGroup}>
          <Activity color="#e63946" size={20} />
          <span style={styles.brandTitle}>HFT CORE</span>
          <span style={styles.envTag}>PRODUCTION TELEMETRY</span>
        </div>

        <div style={styles.statusGroup}>
          <div style={styles.heartbeatPill}>
            <span style={styles.mutedLabel}>LAST PACKET:</span>
            <span style={styles.heartbeatTime}>{lastHeartbeat}</span>
          </div>

          <div
            style={{
              ...styles.connectionPill,
              borderColor: connected ? 'rgba(52, 211, 153, 0.3)' : 'rgba(230, 57, 70, 0.3)',
              background: connected ? 'rgba(52, 211, 153, 0.05)' : 'rgba(230, 57, 70, 0.05)'
            }}
          >
            {connected ? (
              <>
                <Wifi size={14} color="#34d399" />
                <span style={{ color: '#34d399' }}>TCP 8084 ACTIVE</span>
              </>
            ) : (
              <>
                <WifiOff size={14} color="#e63946" />
                <span style={{ color: '#e63946' }}>DISCONNECTED</span>
              </>
            )}
          </div>
        </div>
      </header>

      {/* MAIN TELEMETRY GRID */}
      <main style={styles.grid}>
        
        {/* TOP OF BOOK PANEL */}
        <section style={styles.panel}>
          <div style={styles.panelHeader}>
            <h2 style={styles.panelTitle}>Top Of Book</h2>
            <span style={styles.liveBadge}>LIVE FEED</span>
          </div>

          <div style={styles.bookGrid}>
            {/* BEST BID */}
            <div style={{ ...styles.priceCard, borderColor: '#1d283a' }}>
              <div style={styles.cardHeader}>
                <span style={{ ...styles.sideLabel, color: '#3b82f6' }}>BEST BID</span>
                <span style={styles.mutedLabel}>L1 PRICE</span>
              </div>
              <div style={styles.priceDisplay} className="tabular-nums">
                {bidPrice}
              </div>
            </div>

            {/* BEST ASK */}
            <div style={{ ...styles.priceCard, borderColor: '#3a2b1a' }}>
              <div style={styles.cardHeader}>
                <span style={{ ...styles.sideLabel, color: '#f59e0b' }}>BEST ASK</span>
                <span style={styles.mutedLabel}>L1 PRICE</span>
              </div>
              <div style={styles.priceDisplay} className="tabular-nums">
                {askPrice}
              </div>
            </div>
          </div>

          {/* REAL DERIVED SPREAD */}
          <div style={styles.spreadBanner}>
            <div style={styles.spreadStat}>
              <span style={styles.mutedLabel}>SPREAD VALUE</span>
              <span style={styles.spreadValue} className="tabular-nums">${spreadDollars}</span>
            </div>
            <div style={styles.spreadStat}>
              <span style={styles.mutedLabel}>SPREAD %</span>
              <span style={styles.spreadValue} className="tabular-nums">{spreadPercent}%</span>
            </div>
          </div>
        </section>

        {/* SYSTEM PERFORMANCE & CONTROL PANEL */}
        <section style={styles.panel}>
          <div style={styles.panelHeader}>
            <h2 style={styles.panelTitle}>Engine Telemetry</h2>
            <Zap size={16} color="#e63946" />
          </div>

          <div style={styles.metricsList}>
            {/* INGESTION RATE */}
            <div style={styles.metricRow}>
              <div>
                <div style={styles.metricTitle}>Ingestion Throughput</div>
                <div style={styles.metricSub}>Lock-free SPSC Ringbuffer</div>
              </div>
              <div style={styles.metricValue} className="tabular-nums">
                {data.throughput_mps.toFixed(2)} <span style={styles.unit}>M/s</span>
              </div>
            </div>

            {/* TOTAL ORDERS PROCESSED */}
            <div style={styles.metricRow}>
              <div>
                <div style={styles.metricTitle}>Cumulative Orders</div>
                <div style={styles.metricSub}>Current Trading Session</div>
              </div>
              <div style={styles.metricValue} className="tabular-nums">
                {data.total_orders.toLocaleString()}
              </div>
            </div>
          </div>

          {/* ENGINE KILL SWITCH */}
          <div style={styles.actionContainer}>
            <button
              onClick={handleKillSwitch}
              disabled={!connected}
              style={{
                ...styles.killButton,
                opacity: connected ? 1 : 0.4,
                cursor: connected ? 'pointer' : 'not-allowed'
              }}
            >
              <ShieldAlert size={18} />
              <span>EMERGENCY ENGINE HALT</span>
            </button>
          </div>
        </section>

      </main>
    </div>
  );
}

// --- STYLES ---
const styles: { [key: string]: React.CSSProperties } = {
  container: {
    width: '100vw',
    minHeight: '100vh',
    display: 'flex',
    flexDirection: 'column',
    padding: '24px 32px',
    maxWidth: '1400px',
    margin: '0 auto',
  },
  header: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingBottom: '24px',
    borderBottom: '1px solid #18181b',
    marginBottom: '28px',
  },
  brandGroup: {
    display: 'flex',
    alignItems: 'center',
    gap: '12px',
  },
  brandTitle: {
    fontFamily: "'Space Grotesk', sans-serif",
    fontSize: '1.2rem',
    fontWeight: 700,
    letterSpacing: '1px',
    color: '#ffffff',
  },
  envTag: {
    fontSize: '0.65rem',
    fontWeight: 600,
    color: '#71717a',
    background: '#121215',
    padding: '3px 8px',
    borderRadius: '4px',
    border: '1px solid #27272a',
    letterSpacing: '1px',
  },
  statusGroup: {
    display: 'flex',
    alignItems: 'center',
    gap: '16px',
  },
  heartbeatPill: {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    fontSize: '0.75rem',
  },
  heartbeatTime: {
    fontFamily: "'Inter', sans-serif",
    fontWeight: 600,
    color: '#a1a1aa',
  },
  connectionPill: {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    padding: '6px 14px',
    borderRadius: '20px',
    fontSize: '0.75rem',
    fontWeight: 600,
    border: '1px solid',
    letterSpacing: '0.5px',
  },
  grid: {
    display: 'grid',
    gridTemplateColumns: '1.4fr 1fr',
    gap: '24px',
  },
  panel: {
    background: '#0d0d10',
    borderRadius: '16px',
    padding: '24px',
    border: '1px solid #1a1a20',
    display: 'flex',
    flexDirection: 'column',
    gap: '20px',
  },
  panelHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  panelTitle: {
    fontFamily: "'Space Grotesk', sans-serif",
    fontSize: '1.1rem',
    fontWeight: 600,
    color: '#ffffff',
  },
  liveBadge: {
    fontSize: '0.65rem',
    fontWeight: 700,
    color: '#34d399',
    letterSpacing: '1px',
  },
  bookGrid: {
    display: 'grid',
    gridTemplateColumns: '1fr 1fr',
    gap: '16px',
  },
  priceCard: {
    background: '#060608',
    borderRadius: '12px',
    padding: '20px',
    border: '1px solid',
    display: 'flex',
    flexDirection: 'column',
    gap: '12px',
  },
  cardHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  sideLabel: {
    fontSize: '0.85rem',
    fontWeight: 700,
    letterSpacing: '0.5px',
  },
  mutedLabel: {
    fontSize: '0.7rem',
    color: '#71717a',
    fontWeight: 600,
    letterSpacing: '0.5px',
  },
  priceDisplay: {
    fontSize: '3rem',
    fontWeight: 600,
    color: '#ffffff',
    lineHeight: '1',
  },
  spreadBanner: {
    background: '#060608',
    borderRadius: '12px',
    padding: '16px 20px',
    border: '1px solid #1a1a20',
    display: 'flex',
    justifyContent: 'space-around',
    alignItems: 'center',
  },
  spreadStat: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '4px',
  },
  spreadValue: {
    fontSize: '1.1rem',
    fontWeight: 600,
    color: '#e63946',
  },
  metricsList: {
    display: 'flex',
    flexDirection: 'column',
    gap: '12px',
  },
  metricRow: {
    background: '#060608',
    borderRadius: '12px',
    padding: '16px 20px',
    border: '1px solid #141418',
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  metricTitle: {
    fontSize: '0.9rem',
    fontWeight: 600,
    color: '#ffffff',
  },
  metricSub: {
    fontSize: '0.75rem',
    color: '#71717a',
    marginTop: '2px',
  },
  metricValue: {
    fontSize: '1.4rem',
    fontWeight: 600,
    color: '#ffffff',
  },
  unit: {
    fontSize: '0.85rem',
    color: '#71717a',
    fontWeight: 400,
  },
  actionContainer: {
    marginTop: 'auto',
    paddingTop: '12px',
  },
  killButton: {
    width: '100%',
    background: 'rgba(230, 57, 70, 0.12)',
    color: '#e63946',
    border: '1px solid rgba(230, 57, 70, 0.4)',
    padding: '16px',
    borderRadius: '10px',
    fontSize: '0.85rem',
    fontWeight: 700,
    letterSpacing: '1px',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '10px',
    transition: 'all 0.15s ease',
  },
};