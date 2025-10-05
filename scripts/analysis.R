dir.create("reports", showWarnings = FALSE)

files <- list.files("data", pattern="^leituras_.*\\.csv$", full.names=TRUE)
if (length(files) == 0) stop("Nenhum CSV em data/")

df <- do.call(rbind, lapply(files, function(p) {
  x <- read.csv(p, stringsAsFactors = FALSE)
  x$timestamp <- as.POSIXct(x$timestamp, format="%Y-%m-%dT%H:%M:%SZ", tz="UTC")
  x
}))

png("reports/umidade.png", width=1200, height=500)
plot(df$timestamp, df$pct, type="l", xlab="Tempo (UTC)", ylab="Umidade (%)")
dev.off()

cat("Relatório salvo em reports/umidade.png\n")
write.csv(df, "reports/dados_consolidados.csv", row.names=FALSE)
cat("Dados consolidados salvos em reports/dados_consolidados.csv\n")