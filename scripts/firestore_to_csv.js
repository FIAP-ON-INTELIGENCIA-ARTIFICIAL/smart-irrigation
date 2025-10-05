// scripts/firestore_to_csv.js
// npm i firebase-admin fast-csv
const admin = require("firebase-admin");
const fs = require("fs");
const { format } = require("@fast-csv/format");

admin.initializeApp({ credential: admin.credential.applicationDefault() });
const db = admin.firestore();

(async () => {
  const csvPath = "data/export_firestore.csv";
  fs.mkdirSync("data", { recursive: true });
  const stream = format({ headers: true });
  stream.pipe(fs.createWriteStream(csvPath));

  const snap = await db.collection("leituras").doc("esp32_01")
    .collection("historico") // se você tiver histórico aqui
    // .where("ts", ">=", new Date("2025-10-05T00:00:00Z"))
    .get();

  snap.forEach(doc => {
    const d = doc.data();
    stream.write({
      timestamp: d.ts && d.ts.toDate ? d.ts.toDate().toISOString() : d.ts,
      mv: d.mv, pct: d.pct
    });
  });

  stream.end();
  console.log("CSV salvo em", csvPath);
})();
