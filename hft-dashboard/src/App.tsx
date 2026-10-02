import { useState, useEffect } from 'react';
import { Activity, ArrowDownRight, ArrowUpRight, Server } from 'lucide-react';

// Define the shape of our incoming telemetry data
interface TelemetryData {
  throughput_mps: number;
  best_bid: number;
  best_ask: number;
  total_orders: number;
}

function App() {
  const [data, setData] = useState<TelemetryData>({
    throughput_mps: 0,
    best_bid: 0,
    best_ask: 0,
    total_orders: 0
  });
  const [connected, setConnected] = useState<boolean>(false);

  useEffect(() => {
    // Connect to the Python WebSocket Bridge on the new port 8084
    const ws = new WebSocket('ws://localhost:8084');

    ws.onopen = () => setConnected(true);
    
    ws.onmessage = (event) => {
      try {
        const parsedData: TelemetryData = JSON.parse(event.data);
        setData(parsedData);
      } catch (e) {
        console.error("Failed to parse telemetry data", e);
      }
    };

    ws.onclose = () => setConnected(false);

    return () => ws.close();
  }, []);

  const styles = {
    container: { minHeight: '100vh', backgroundColor: '#0f172a', color: 'white', padding: '2rem', fontFamily: 'system-ui, sans-serif' },
    header: { display: 'flex', alignItems: 'center', gap: '10px', marginBottom: '2rem', borderBottom: '1px solid #1e293b', paddingBottom: '1rem' },
    grid: { display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(250px, 1fr))', gap: '1.5rem' },
    card: { backgroundColor: '#1e293b', padding: '1.5rem', borderRadius: '8px', border: '1px solid #334155' },
    label: { color: '#94a3b8', fontSize: '0.875rem', fontWeight: 'bold', textTransform: 'uppercase' as const },
    value: { fontSize: '2rem', fontWeight: 'bold', margin: '0.5rem 0' },
    statusOk: { color: '#10b981', display: 'flex', alignItems: 'center', gap: '5px', fontSize: '0.875rem' },
    statusFail: { color: '#ef4444', display: 'flex', alignItems: 'center', gap: '5px', fontSize: '0.875rem' },
    green: { color: '#10b981' },
    red: { color: '#ef4444' }
  };

  return (
    <div style={styles.container}>
      <div style={styles.header}>
        <Activity size={32} color="#3b82f6" />
        <h1 style={{ margin: 0 }}>HFT Gateway Telemetry</h1>
        <div style={{ marginLeft: 'auto' }}>
          {connected ? 
            <span style={styles.statusOk}><Server size={16}/> System LIVE</span> : 
            <span style={styles.statusFail}><Server size={16}/> DISCONNECTED</span>
          }
        </div>
      </div>

      <div style={styles.grid}>
        <div style={styles.card}>
          <div style={styles.label}>Ingestion Throughput</div>
          <div style={styles.value}>{data.throughput_mps.toFixed(2)} M/sec</div>
          <div style={{ color: '#94a3b8', fontSize: '0.875rem' }}>Orders per second</div>
        </div>

        <div style={styles.card}>
          <div style={styles.label}>Best Bid (Buy)</div>
          <div style={{...styles.value, ...styles.green}}>
            ${(data.best_bid / 100).toFixed(2)}
          </div>
          <div style={styles.statusOk}><ArrowUpRight size={16}/> Top of Book</div>
        </div>

        <div style={styles.card}>
          <div style={styles.label}>Best Ask (Sell)</div>
          <div style={{...styles.value, ...styles.red}}>
            ${(data.best_ask / 100).toFixed(2)}
          </div>
          <div style={styles.statusFail}><ArrowDownRight size={16}/> Top of Book</div>
        </div>

        <div style={styles.card}>
          <div style={styles.label}>Total Orders Processed</div>
          <div style={styles.value}>{data.total_orders.toLocaleString()}</div>
          <div style={{ color: '#94a3b8', fontSize: '0.875rem' }}>Session cumulative</div>
        </div>
      </div>
    </div>
  );
}

export default App;