# scripts/analysis.R
options(stringsAsFactors = FALSE)

dir.create("reports", showWarnings = FALSE)

files <- list.files("data", pattern="^leituras_.*\\.csv$", full.names=TRUE)
if (length(files) == 0) stop("Nenhum CSV encontrado em data/")

# Leitor tolerante a formatos (CSV antigo e CSVX)
read_one <- function(p) {
  x <- tryCatch(read.csv(p, check.names = FALSE), error = function(e) NULL)
  if (is.null(x) || nrow(x) == 0) return(NULL)

  # Padroniza colunas
  # timestamp
  if (!"timestamp" %in% names(x)) {
    # às vezes o cabeçalho veio com "ts" (CSVX)
    if ("ts" %in% names(x)) names(x)[names(x) == "ts"] <- "timestamp"
  }

  # mv/soil_mv
  if (!"mv" %in% names(x) && "soil_mv" %in% names(x)) x$mv <- x$soil_mv
  if (!"soil_mv" %in% names(x) && "mv" %in% names(x)) x$soil_mv <- x$mv

  # pct/soil_pct
  if (!"pct" %in% names(x) && "soil_pct" %in% names(x)) x$pct <- x$soil_pct
  if (!"soil_pct" %in% names(x) && "pct" %in% names(x)) x$soil_pct <- x$pct

  # ldr_mv/ph podem não existir em CSV antigo
  if (!"ldr_mv" %in% names(x)) x$ldr_mv <- NA_real_
  if (!"ph" %in% names(x))     x$ph     <- NA_real_

  # flags e meteo podem não existir
  for (nm in c("pump_on","pump_allowed","rain_hold","wx_code","wx_wet",
               "npk_n","npk_p","npk_k")) {
    if (!nm %in% names(x)) x[[nm]] <- NA
  }

  # timestamp -> POSIXct (ISO UTC)
  if ("timestamp" %in% names(x)) {
    # tenta ISO (YYYY-mm-ddTHH:MM:SSZ). Se falhar, tenta outros formatos
    ts <- suppressWarnings(as.POSIXct(x$timestamp, format="%Y-%m-%dT%H:%M:%SZ", tz="UTC"))
    # fallback: tenta ler se vier com fuso/locale
    bad <- is.na(ts)
    if (any(bad)) {
      ts2 <- suppressWarnings(as.POSIXct(x$timestamp[bad], tz="UTC", tryFormats=c(
        "%Y-%m-%d %H:%M:%S", "%d/%m/%Y %H:%M:%S", "%Y-%m-%dT%H:%M:%S"
      )))
      ts[bad] <- ts2
    }
    x$timestamp <- ts
  } else {
    x$timestamp <- NA
  }

  # mantém apenas colunas de interesse (ordem fixa)
  want <- c("timestamp","soil_mv","soil_pct","mv","pct","ldr_mv","ph",
            "npk_n","npk_p","npk_k","pump_on","pump_allowed","rain_hold",
            "wx_code","wx_wet")
  miss <- setdiff(want, names(x))
  for (nm in miss) x[[nm]] <- NA
  x[want]
}

lst <- lapply(files, read_one)
lst <- Filter(Negate(is.null), lst)
if (length(lst) == 0) stop("Não consegui ler nenhum CSV válido em data/")

df <- do.call(rbind, lst)
# ordena por tempo e remove timestamps NA
df <- df[!is.na(df$timestamp), ]
df <- df[order(df$timestamp), ]

if (nrow(df) == 0) stop("Sem dados com timestamp válido.")

# Função de plot segura
safe_plot <- function(x, y, ylab, file) {
  ok <- complete.cases(x, y)
  if (sum(ok) < 2) {
    message(sprintf("[AVISO] Poucos dados para %s. Pulando gráfico.", ylab))
    return(invisible(FALSE))
  }
  png(file, width=1200, height=500)
  plot(x[ok], y[ok], type="l", xlab="Tempo (UTC)", ylab=ylab)
  grid()
  dev.off()
  message("Gerado: ", file)
  TRUE
}

# Gráficos
safe_plot(df$timestamp, df$soil_pct, "Umidade do Solo (%)", "reports/umidade.png")
safe_plot(df$timestamp, df$ldr_mv,   "LDR (mV)",           "reports/ldr_mv.png")
safe_plot(df$timestamp, df$ph,       "pH (estimado)",      "reports/ph.png")

# Estado da bomba (0/1)
if (!all(is.na(df$pump_on))) {
  y <- as.integer(df$pump_on)
  safe_plot(df$timestamp, y, "Bomba (0=OFF,1=ON)", "reports/pump.png")
}

# Meteo (wx_code como degrau)
if (!all(is.na(df$wx_code))) {
  y <- as.numeric(df$wx_code)
  safe_plot(df$timestamp, y, "WMO weather code", "reports/wx_code.png")
}

# Exporta consolidado
write.csv(df, "reports/dados_consolidados.csv", row.names = FALSE)
cat("Dados consolidados salvos em reports/dados_consolidados.csv\n")
cat("Análise concluída.\n")