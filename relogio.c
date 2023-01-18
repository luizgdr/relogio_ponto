#define __USE_XOPEN
#define _GNU_SOURCE
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cada registro tera um horario de inicio e fim, um indicador se o registro
 * esta ativo, e um indicador se o registro tem o horario de inicio e fim
 * */
typedef struct {
  struct tm inicio;
  struct tm fim;
  int ativo;
  int completo;
} registro;
/* Cada diario tera os registros do dia, o dia e a quantidade de registros
 * */
typedef struct {
  struct tm data;
  registro *registros;
  int tamanho;
  int idx;
} diario;

int toggle(registro *reg);
void status(registro *reg);
void imprimir_log(diario *dia, diario *ant, int ant_qtde);
struct tm *_log(diario *dia);
struct tm *calcular_diff(struct tm *t1, struct tm *t0);
struct tm *now(void);
struct tm *imprimir_registro(registro *reg);
void escrever_log(const char *fmt, ...);
void novo_registro(diario *dia);
void zerar_struct_tm(struct tm *tm);
void checar_log(diario *dia, diario *ant, int *ant_qtde);
void ler_log(FILE *fp, diario *dia, const char *str_data, char *buf, size_t bufsize, ssize_t nbytes);

/* Função que controla a parada e inicialização da contagem de horas
 * Retorna verdadeiro quando o registro atual está completo
 * */
int
toggle(registro *reg)
{
  if (reg->completo) return 0;

  /* Guardando o horario atual */
  struct tm *data = now();

  if (reg->ativo) {
    reg->fim = *data;
    reg->completo = 1;
    char str_fim[6];
    strftime(str_fim, 6, "%k:%M", &reg->fim);
    printf("Registro finalizado as %s\n", str_fim);
    escrever_log(";%s\n", str_fim);
    /* Registro completo */
    return 1;
  } else {
    reg->inicio = *data;
    reg->ativo = 1;
    char str_inicio[6];
    strftime(str_inicio, 6, "%k:%M", &reg->inicio);
    printf("Registro comecado as %s\n", str_inicio);
    escrever_log("%s", str_inicio);
    return 0;
  }
}

/* Função que mostra para o usuário a quantidade de horas de um registro
 * */
void
status(registro *reg)
{
  if (!reg->ativo) {
    puts("Registro não ativo");
    return;
  }

  imprimir_registro(reg);
}

/* Função que itera pelos diários do log, mostrando os para o usuário
 * Mostra a soma total das horas mensais
 * */
void
imprimir_log(diario *dia, diario *ant, int ant_qtde)
{
  time_t soma_total = 0;
  for (int i = 0; i < ant_qtde; i += 1) {
    struct tm *valor = _log(&ant[i]);
    if (valor) {
      soma_total += valor->tm_hour * 3600 + valor->tm_min * 60;
    }
  }
  struct tm *valor = _log(dia);
  if (valor) {
    soma_total += valor->tm_hour * 3600 + valor->tm_min * 60;
  }
  struct tm *soma_tm = gmtime(&soma_total);
  printf("\tTotal Mensal: %d:%02d\n",
    soma_tm->tm_yday * 24 + soma_tm->tm_hour,
    soma_tm->tm_min
  );
}

/* Função que mostra para o usuário todos os registros do diário,
 * acompanhado da soma de todos os registros e o dia dos registros
 * Retorna a soma dos registros do diário
 * */
struct tm *
_log(diario *dia)
{
  char str_data[11];
  strftime(str_data, 11, "%d/%m/%Y", &dia->data);
  puts(str_data);
  time_t soma = 0;
  for (int i = 0; i < dia->tamanho; i += 1) {
    registro *atual = &(dia->registros)[i];
    struct tm *diff = imprimir_registro(atual);
    if (diff) {
      /* Únicos campos importantes são as horas e os minutos */
      soma += diff->tm_hour * 3600 + diff->tm_min * 60;
    }
  }
  struct tm *soma_tm = gmtime(&soma);
  char str_soma[6];
  strftime(str_soma, 6, "%k:%M", soma_tm);
  printf("\tTotal: %s\n", str_soma);
  return soma_tm;
}

/* Função para calcular a diferença entre dois horários
 * Retorna o valor do cálculo
 * */
struct tm *
calcular_diff(struct tm *t1, struct tm *t0)
{
  time_t time1 = mktime(t1);
  time_t time0 = mktime(t0);
  time_t diff = (time_t)difftime(time1, time0);
  return gmtime(&diff);
}

/* Função para obter o horário atual do sistema, descartando os segundos
 * Retorna o horário atual do sistema
 * */
struct tm *
now()
{
  time_t _now = time(NULL);
  struct tm* ret = localtime(&_now);
  /* Descartar segundos */
  ret->tm_sec = 0;
  return ret;
}

/* Função auxiliar para mostrar o atual estado de um registro
 * Retorna a diferença entre os horários do registro (Usado para o log)
 * */
struct tm *
imprimir_registro(registro *reg)
{
  if (!reg->ativo) return NULL;

  struct tm *desde, *ate, *diff;
  desde = &reg->inicio;

  int _free = 0;
  if (reg->completo) {
    ate = &reg->fim;
  } else {
    /* Alocar dinâmicamente, pois a função now() retorna um ponteiro
     * para memória alocada estaticamente que pode ser sobreescrita ao
     * decorrer do programa
     * */
    struct tm *tmp = malloc(sizeof(*tmp));
    _free = 1;
    ate = now();
    *tmp = *ate;
    ate = tmp;
  }
  diff = calcular_diff(ate, desde);
  /* Converter para string */
  char str_desde[6], str_ate[6], str_diff[6];
  strftime(str_desde, 6, "%k:%M", desde);
  strftime(str_ate, 6, "%k:%M", ate);
  strftime(str_diff, 6, "%k:%M", diff);
  printf("%-6s-> %s | %s\n", str_desde, str_ate, str_diff);

  if (_free) {
    free(ate);
  }
  return diff;
}

/* Função que escreve no arquivo de log informações sobre o registro
 * */
void
escrever_log(const char *fmt, ...)
{
  FILE *fp = fopen("log.txt", "a");
  if (!fp) {
    fprintf(stderr, "Erro na escritura do arquivo de log\n");
    exit(1);
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(fp, fmt, args);
  va_end(args);

  fclose(fp);
}

/* Função que aloca espaço para um novo registro na memória do programa e
 * muda o atual registro para o novo registro alocado
 * */
void
novo_registro(diario *dia)
{
  registro *reg = realloc(dia->registros, sizeof(*reg) * (dia->tamanho + 1));
  dia->registros = reg;
  if (!dia->registros) {
    fprintf(stderr, "Erro realocando registros\n");
    exit(2);
  }
  dia->tamanho += 1;

  dia->idx = dia->tamanho - 1;
  registro *atual = &(dia->registros)[dia->idx];
  atual->ativo = 0;
  atual->completo = 0;
  zerar_struct_tm(&atual->inicio);
  zerar_struct_tm(&atual->fim);
}

/* Função para zerar o struct tm recém-alocado
 * */
void
zerar_struct_tm(struct tm *tm)
{
  tm->tm_gmtoff = 0;
  tm->tm_hour = 0;
  tm->tm_isdst = 0;
  tm->tm_mday = 0;
  tm->tm_min = 0;
  tm->tm_mon = 0;
  tm->tm_sec = 0;
  tm->tm_wday = 0;
  tm->tm_yday = 0;
  tm->tm_year = 0;
  tm->tm_zone = 0;
}

/* Função que checa se um diário deste dia já existe,
 * se não existe ainda, então escreve no log o dia deste diário
 * Inicializa registros se existirem no dia
 * */
void
checar_log(diario *dia, diario *ant, int *ant_qtde)
{
  /* Dia em formato de string */
  char str_data[11];
  strftime(str_data, 11, "%d/%m/%Y", &dia->data);

  FILE *fp = fopen("log.txt", "r");
  if (!fp) {
    escrever_log("%s\n", str_data);
    return;
  }

  int existe = 0;

  char *buf = NULL;
  size_t bufsize = 0;
  ssize_t nbytes;
  /* Ler linha por linha */
  while ((nbytes = getline(&buf, &bufsize, fp)) != -1) {
    /* Mesmo dia */
    if (strstr(buf, str_data)) {
      existe = 1;
      break;
    /* Mesmo mês */
    } else if (strstr(buf, str_data + 3)) {
      diario *diario = &ant[*ant_qtde];
      strptime(buf, "%d/%m/%Y", &diario->data);
      novo_registro(diario);

      char _str_data[11];
      snprintf(_str_data, 11, "%s", buf);
      FILE tmp = *fp;
      ler_log(&tmp, diario, _str_data, buf, bufsize, nbytes);
      *ant_qtde += 1;
    }
  }

  if (existe) {
    ler_log(fp, dia, str_data, buf, bufsize, nbytes);
  } else {
    escrever_log("%s\n", str_data);
  }

  free(buf);
  fclose(fp);
}

/* Função que inicializa registros lendo informações do log
 * */
void
ler_log(FILE *fp, diario *dia, const char *str_data, char *buf, size_t bufsize, ssize_t nbytes)
{
  FILE *pv = fp;
  while ((nbytes = getline(&buf, &bufsize, fp)) != -1) {
    if (buf[strcspn(buf, "/")] == '/') {
      fp = pv;
      return;
    }
    int idx = strcspn(buf, ";");
    registro *atual = &(dia->registros)[dia->idx];
    if (idx == strlen(buf)) {
      atual->ativo = 1;
      char str_registro[20];
      snprintf(str_registro, 20, "%s %s", buf, str_data);
      strptime(str_registro, "%H:%M %d/%m/%Y", &atual->inicio);
      break;
    } else {
      atual->ativo = 1;
      atual->completo = 1;

      buf[idx] = '\0';
      char str_registro[20];
      snprintf(str_registro, 20, "%s %s", buf, str_data);
      strptime(str_registro, "%H:%M %d/%m/%Y", &atual->inicio);

      snprintf(str_registro, 20, "%s %s", buf+idx+1, str_data);
      strptime(str_registro, "%H:%M %d/%m/%Y", &atual->fim);
      novo_registro(dia);
    }
    pv = fp;
  }
}

int
main(int argc, char *argv[])
{
  diario anteriores[30] = {0};
  int ant_qtde = 0;

  diario diario = {0};
  diario.data = *now();
  novo_registro(&diario);
  checar_log(&diario, anteriores, &ant_qtde);

  int sair = 0;
  while (!sair) {
    char c;
    /* Pular quebra de linhas */
    do {
      c = getc(stdin);
    } while (c == '\n' || c == '\r');
    switch (c) {
    case 't':
      if (toggle(&(diario.registros)[diario.idx])) {
        novo_registro(&diario);
      }
      break;
    case 's':
      status(&(diario.registros)[diario.idx]);
      break;
    case 'l':
      imprimir_log(&diario, anteriores, ant_qtde);
      break;
    case 'q':
      sair = 1;
      break;
    default:
      puts("Comando inválido");
    case 'h': /* FALLTHROUGH */
      puts("\
`t` para iniciar ou parar a contagem de um registro\n\
`s` para mostrar o status do atual registro\n\
`l` para mostrar o log dos regisros mensais\n\
`h` para mostrar esta ajuda\n\
`q` para sair");
    }
  }
  free(diario.registros);
  for (int i = 0; i < ant_qtde; i += 1) {
    free(anteriores[i].registros);
  }
  return 0;
}
