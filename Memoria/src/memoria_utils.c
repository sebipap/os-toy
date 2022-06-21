#include <memoria_utils.h>
#include <main.h>
#include <swap.h>

Logger *iniciar_logger_memoria()
{
  return log_create("Memoria.log", "Memoria", true, LOG_LEVEL_INFO);
}

int iniciar_servidor_memoria()
{
  return iniciar_servidor(MEMORIA_CONFIG.IP_MEMORIA, MEMORIA_CONFIG.PUERTO_MEMORIA);
}

// Estructuras
void iniciar_estructuras_memoria()
{
  Logger *logger = iniciar_logger_memoria();

  tablasDePrimerNivel = 0;
  memoriaPrincipal = (void *)malloc(MEMORIA_CONFIG.TAM_MEMORIA);
  int cantidadMarcos = MEMORIA_CONFIG.TAM_MEMORIA / MEMORIA_CONFIG.TAM_PAGINA;
  iniciar_marcos(cantidadMarcos);
  procesos = list_create();

  log_info(logger, "Estructuras de memoria inicializadas");
  log_destroy(logger);
}

void iniciar_marcos(int cantidadMarcos)
{
  marcos = list_create();
  for (int i = 0; i < cantidadMarcos; i++)
  {
    Marco *marco = malloc(sizeof(Marco));
    marco->idProceso = -1;
    marco->paginaActual = NULL;
    list_add(marcos, marco);
  }
}

Proceso *crear_proceso(int id, int tamanio)
{
  Logger *logger = iniciar_logger_memoria();

  Proceso *proceso = malloc(sizeof(Proceso));
  proceso->idProceso = id;
  proceso->tamanio = tamanio;
  proceso->tablaPrimerNivel = crear_tabla_primer_nivel();

  log_info(logger, "Se creo el Proceso: %d, tamanio: %d", proceso->idProceso, proceso->tamanio);

  proceso->archivoSwap = crear_archivo_swap(id);

  list_add(procesos, proceso);

  log_destroy(logger);

  return proceso;
}

TablaPrimerNivel *crear_tabla_primer_nivel()
{
  TablaPrimerNivel *tabla = malloc(sizeof(TablaPrimerNivel));
  tabla->entradas = list_create();
  tabla->nroTablaPrimerNivel = tablasDePrimerNivel;
  tablasDePrimerNivel++;

  int entradasPorTabla = MEMORIA_CONFIG.ENTRADAS_POR_TABLA;
  for (int numeroTablaSegundoNivel = 0; numeroTablaSegundoNivel < entradasPorTabla; numeroTablaSegundoNivel++)
  {
    TablaSegundoNivel *tablaSegundoNivel = crear_tabla_segundo_nivel(numeroTablaSegundoNivel * entradasPorTabla);
    list_add(tabla->entradas, tablaSegundoNivel);
  }

  return tabla;
}

TablaSegundoNivel *crear_tabla_segundo_nivel(int nroPrimerPaginaDeTabla)
{
  TablaSegundoNivel *tabla = malloc(sizeof(TablaSegundoNivel));
  tabla->entradas = list_create();

  int entradasPorTabla = MEMORIA_CONFIG.ENTRADAS_POR_TABLA;
  for (int i = 0; i < entradasPorTabla; i++)
  {
    Pagina *pag = malloc(sizeof(Pagina));
    pag->nroPagina = i + nroPrimerPaginaDeTabla;
    pag->modificado = false;
    pag->uso = true;
    pag->marcoAsignado = NULL;
    list_add(tabla->entradas, pag);
  }
  return tabla;
}
//

Proceso *buscar_proceso_por_id(int idProceso)
{

  bool es_proceso(void *_proceso)
  {
    Proceso *proceso = (Proceso *)_proceso;
    return proceso->idProceso == idProceso;
  }

  return list_find(procesos, &es_proceso);
}

void escribir_memoria(uint32_t valorAEscribir, int direccionFisica)
{
  Logger *logger = iniciar_logger_memoria();

  void *desplazamiento = memoriaPrincipal + direccionFisica;
  memcpy(desplazamiento, &valorAEscribir, sizeof(uint32_t));

  log_info(logger, "Valor escrito %d, en la posicion de memoria fisica %d.", valorAEscribir, direccionFisica);
  log_destroy(logger);
}

uint32_t leer_de_memoria(int direccionFisica)
{
  Logger *logger = iniciar_logger_memoria();

  uint32_t leido;
  void *desplazamiento = memoriaPrincipal + direccionFisica;
  memcpy(&leido, desplazamiento, sizeof(uint32_t));

  log_info(logger, "Valor leido %d, en la posicion de memoria fisica %d.", leido, direccionFisica);
  log_destroy(logger);

  return leido;
}

void copiar_en_memoria(int direccionFisicaDestino, int direccionFisicaOrigen)
{
  Logger *logger = iniciar_logger_memoria();

  log_info(logger, "Iniciando Copia:");

  uint32_t leido = leer_de_memoria(direccionFisicaOrigen);
  escribir_memoria(leido, direccionFisicaDestino);

  log_info(logger, "Copia finalizada.");
  log_destroy(logger);
}

Marco *primer_marco_libre() // First fit
{
  bool marco_libre(void *_marco)
  {
    Marco *marco = (Marco *)_marco;
    return marco->paginaActual == NULL;
  }

  return list_find(marcos, &marco_libre);
}

bool tiene_marcos_por_asignar(Proceso *proceso)
{
  bool tiene_pagina_asignada_del_proceso(void *_marco)
  {
    Marco *marco = (Marco *)_marco;
    return marco->idProceso == proceso->idProceso;
  }

  int marcosAsignados = list_count_satisfying(marcos, &tiene_pagina_asignada_del_proceso);

  return marcosAsignados < MEMORIA_CONFIG.MARCOS_POR_PROCESO;
}

int numero_de_marco(Marco *marco)
{
  for (int i = 0; i < list_size(marcos); i++)
  {
    if (list_get(marcos, i) == marco)
    {
      return i;
    }
  }

  return -1;
}

void asignar_pagina_del_proceso_al_marco(int idProceso, Pagina *pagina, Marco *marco)
{
  Logger *logger = iniciar_logger_memoria();

  pagina->marcoAsignado = marco;
  marco->paginaActual = pagina;
  marco->idProceso = idProceso;

  log_info(logger, "Pagina %d del proceso %d, asignada al Marco %d", pagina->nroPagina, idProceso, numero_de_marco(marco));
  log_destroy(logger);
}

Pagina *obtener_pagina_del_proceso(Proceso *proceso, int nroPagina) // Tambien se podria buscar el nroPagina en las tablas del proceso, uso cuenta de la MMU
{
  // double numeroEntradasPrimerNivel = nroPagina / MEMORIA_CONFIG.ENTRADAS_POR_TABLA;
  int numeroDeEntradaDelPrimerNivel = floor(nroPagina / MEMORIA_CONFIG.ENTRADAS_POR_TABLA);
  int numeroDeEntradaDelSegundoNivel = nroPagina % MEMORIA_CONFIG.ENTRADAS_POR_TABLA;

  TablaSegundoNivel *tablaSegundoNivel = list_get(proceso->tablaPrimerNivel->entradas, numeroDeEntradaDelPrimerNivel);

  return list_get(tablaSegundoNivel->entradas, numeroDeEntradaDelSegundoNivel);
}

Marco *asignar_pagina_a_marco_libre(Proceso *proceso, int nroPagina) // TODO: Ver cuando se llamaria
{
  Pagina *pagina = obtener_pagina_del_proceso(proceso, nroPagina);
  Marco *marco;

  if (tiene_marcos_por_asignar(proceso))
  {
    marco = primer_marco_libre();
    if (marco != NULL)
    {
      asignar_pagina_del_proceso_al_marco(proceso->idProceso, pagina, marco);
    }
    else // memoria llena
    {
      // correr algortimo sustitucion
    }
  }
  else // tiene asignada la cantidad maxima de marcos por proceso
  {
    // correr algortimo sustitucion
  }

  return marco;
}

void desasignar_marco(Marco *marco)
{
  marco->paginaActual == NULL;
  marco->idProceso = -1;
}

void suspender_proceso(int idProcesoASuspender)
{
  Proceso *proceso = buscar_proceso_por_id(idProcesoASuspender);

  int entradas = MEMORIA_CONFIG.ENTRADAS_POR_TABLA;
  for (int numeroDeTablaDeSegundoNivel = 0; numeroDeTablaDeSegundoNivel < entradas; numeroDeTablaDeSegundoNivel++)
  {
    TablaSegundoNivel *tablaSegundoNivel = list_get(proceso->tablaPrimerNivel->entradas, numeroDeTablaDeSegundoNivel);

    for (int numeroPaginaSegundoNivel = 0; numeroPaginaSegundoNivel < entradas; numeroPaginaSegundoNivel++)
    {
      Pagina *pagina = list_get(tablaSegundoNivel->entradas, numeroPaginaSegundoNivel);
      Marco *marcoDeLaPagina = pagina->marcoAsignado;
      if (marcoDeLaPagina != NULL && pagina->modificado)
      {
        escribir_en_swap(numero_de_marco(marcoDeLaPagina), proceso);
        desasignar_marco(marcoDeLaPagina);
      }
    }
  }
}

void borrar_tablas_del_proceso(Proceso *proceso)
{
  int entradas = MEMORIA_CONFIG.ENTRADAS_POR_TABLA;
  for (int numeroDeTablaDeSegundoNivel = 0; numeroDeTablaDeSegundoNivel < entradas; numeroDeTablaDeSegundoNivel++)
  {
    TablaSegundoNivel *tablaSegundoNivel = list_get(proceso->tablaPrimerNivel->entradas, numeroDeTablaDeSegundoNivel);

    for (int numeroPaginaSegundoNivel = 0; numeroPaginaSegundoNivel < entradas; numeroPaginaSegundoNivel++)
    {
      Pagina *pagina = list_get(tablaSegundoNivel->entradas, numeroPaginaSegundoNivel);

      free(pagina);
    }

    free(tablaSegundoNivel);
  }

  free(proceso->tablaPrimerNivel);
}

void eliminar_proceso_de_lista_de_procesos(Proceso *proceso)
{
  bool cmpProceso(void *_procesoLista)
  {
    Proceso *procesoLista = (Proceso *)_procesoLista;
    return procesoLista == proceso;
  }

  list_remove_and_destroy_by_condition(procesos, &cmpProceso, &free);
}

void desasignar_marcos_al_proceso(int idProceso)
{
  void limpiar_marco_si_lo_tiene_el_proceso(void *_marco)
  {
    Marco *marco = (Marco *)_marco;
    if (marco->idProceso == idProceso)
    {
      desasignar_marco(marco);
    }
  }

  list_iterate(marcos, &limpiar_marco_si_lo_tiene_el_proceso);
}

void finalizar_proceso(int idProcesoAFinalizar)
{
  Logger *logger = iniciar_logger_memoria();
  Proceso *proceso = buscar_proceso_por_id(idProcesoAFinalizar);

  borrar_tablas_del_proceso(proceso);
  borrar_archivo_swap_del_proceso(proceso);
  eliminar_proceso_de_lista_de_procesos(proceso);
  desasignar_marcos_al_proceso(idProcesoAFinalizar);

  log_info(logger, "Proceso %d finalizado", idProcesoAFinalizar);
  log_destroy(logger);
}

void liberar_memoria()
{
  Logger *logger = iniciar_logger_memoria();

  list_destroy_and_destroy_elements(marcos, &free);

  free(memoriaPrincipal);
  free(procesos);

  log_info(logger, "Estructuras de memoria liberadas");
  log_destroy(logger);
}