#include <swap.h>

char *generar_path_archivo_swap(int idProceso)
{
  return string_from_format("%s%s%s%s", MEMORIA_CONFIG.PATH_SWAP, "/", string_itoa(idProceso), ".swap");
}

FILE *crear_archivo_swap(int idProceso)
{
  Logger *logger = iniciar_logger_memoria();
  ;

  char *pathArchivoSwap = generar_path_archivo_swap(idProceso);

  log_info(logger, "Se creo el archivo de swap del proceso %d en el path %s", idProceso, pathArchivoSwap);
  log_destroy(logger);

  return fopen(pathArchivoSwap, "w+");
}

void borrar_archivo_swap_del_proceso(Proceso *proceso)
{
  fclose(proceso->archivoSwap);
  remove(generar_path_archivo_swap(proceso->idProceso));
}

void escribir_en_swap(int numeroDeMarco, Proceso *proceso)
{
  // TODO
}