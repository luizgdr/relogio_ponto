#define __USE_XOPEN /* strptime(3) */
#define _GNU_SOURCE /* strptime(3) */
#include <time.h>
#include <stdarg.h> /* va_args */
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
int entrada_horas(const char *str_data, registro *reg);
void entrada(const char *str_data, diario *dia);
void salvar_entrada(const char *str_data, const char *fmt, ...);
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
diario *escolher_data(char *str_data, diario *dia, diario *ant, int ant_qtde);

char nome[50] = "";

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

int
entrada_horas(const char *str_data, registro *reg)
{
  char str_inicio[6];
  puts("Qual o horário de início? [Formato HH:MM]");
  scanf("%5s", str_inicio);
  char str_registro[20];
  snprintf(str_registro, 20, "%s %s", str_inicio, str_data);
  char *ret = strptime(str_registro, "%H:%M %d/%m/%Y", &reg->inicio);
  /* O valor de retorno não apontar para o byte nulo da string significa
   * que houve um problema na análise da string
   * */
  if (!ret || *ret) {
    puts("Erro na entrada do horário de início");
    return 1;
  }

  char str_fim[6];
  puts("Qual o horário de saída? [Formato HH:MM]");
  scanf("%5s", str_fim);
  snprintf(str_registro, 20, "%s %s", str_fim, str_data);
  ret = strptime(str_registro, "%H:%M %d/%m/%Y", &reg->fim);
  if (!ret || *ret) {
    puts("Erro na entrada do horário de saída");
    return 1;
  }
  return 0;
}

void
entrada(const char *str_data, diario *dia)
{
  puts(str_data);
  if ((dia->registros)[dia->idx].ativo) novo_registro(dia);
  registro *atual = &(dia->registros)[dia->idx];

  if (entrada_horas(str_data, atual)) {
    return;
  }

  atual->completo = 1;
  atual->ativo = 1;

  status(atual);
  puts("Isso está correto? [s/n]");
  char c;
  do {
    c = getc(stdin);
  } while (c == '\n' || c == '\r');
  if (c == 's' || c == 'S') {
    puts("Entrada de registro completa");
    char str_inicio[6], str_fim[6];
    strftime(str_inicio, 6, "%k:%M", &atual->inicio);
    strftime(str_fim, 6, "%k:%M", &atual->fim);
    salvar_entrada(str_data, "%s;%s\n", str_inicio, str_fim);
    /* Alocar novo registro caso entrada seja no dia atual */
    if (dia->data.tm_yday == now()->tm_yday && \
      dia->data.tm_year == now()->tm_year)
      novo_registro(dia);
  } else {
    puts("Entrada de registro cancelada");
    atual->completo = 0;
    atual->ativo = 0;
    zerar_struct_tm(&atual->inicio);
    zerar_struct_tm(&atual->fim);
  }
}

void
salvar_entrada(const char *str_data, const char *fmt, ...)
{
  char filename[60];
  snprintf(filename, 60, "log-%s.txt", nome);
  FILE *fp = fopen(filename, "r");

  char tmpname[60];
  snprintf(tmpname, 60, "tmp-%s.txt", nome);
  FILE *tmp = fopen(tmpname, "w");

  char *buf = NULL;
  size_t bufsize = 0;
  ssize_t nbytes;
  while ((nbytes = getline(&buf, &bufsize, fp)) != -1) {
    fputs(buf, tmp);
    if (strstr(buf, str_data)) {
      va_list args;
      va_start(args, fmt);
      vfprintf(tmp, fmt, args);
      va_end(args);
    }
  }
  free(buf);
  fclose(fp);
  fclose(tmp);
  rename(tmpname, filename);
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

  /* Liberar memória se alocada dinâmicamente
   * */
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
  char filename[60];
  snprintf(filename, 60, "log-%s.txt", nome);
  FILE *fp = fopen(filename, "a");
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

  char filename[60];
  snprintf(filename, 60, "log-%s.txt", nome);
  FILE *fp = fopen(filename, "r");
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
      ler_log(fp, diario, _str_data, buf, bufsize, nbytes);
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
  char filename[60];
  snprintf(filename, 60, "log-%s.txt", nome);
  FILE *tmp = fopen(filename, "r");
  fseek(tmp, ftell(fp), SEEK_SET);
  while ((nbytes = getline(&buf, &bufsize, tmp)) != -1) {
    if (buf[strcspn(buf, "/")] == '/') {
      fclose(tmp);
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
  }
  fclose(tmp);
}

diario *
escolher_data(char *str_data, diario *dia, diario *ant, int ant_qtde)
{
  char str_datas[31][11];
  puts("Qual dia?");
  int i = 0;
  for (; i < ant_qtde; i += 1) {
    strftime(str_datas[i], 11, "%d/%m/%Y", &ant[i].data);
    printf("%-2d %s\n", i, str_datas[i]);
  }
  strftime(str_datas[i], 11, "%d/%m/%Y", &dia->data);
  printf("%-2d %s\n", i, str_datas[i]);
  int e;
  scanf("%d", &e);
  if (e < i) {
    strcpy(str_data, str_datas[e]);
    return &ant[e];
  } else {
    strcpy(str_data, str_datas[i]);
    return dia;
  }
}

int
main(int argc, char *argv[])
{
  puts("Qual o seu nome? [50 char max]");
  scanf("%49[^\r\n]", nome);
  if (!strlen(nome)) {
    puts("Nenhum nome inserido");
    return 0;
  }

  printf("Bem-vindo %s\n", nome);
  diario anteriores[30] = {0};
  int ant_qtde = 0;

  diario _diario = {0};
  _diario.data = *now();
  novo_registro(&_diario);
  checar_log(&_diario, anteriores, &ant_qtde);

  int sair = 0;
  diario *diario_entrada;
  char str_data[11];
  while (!sair) {
    char c;
    /* Pular quebra de linhas */
    do {
      c = getc(stdin);
    } while (c == '\n' || c == '\r');
    switch (c) {
    case 't':
      if (toggle(&(_diario.registros)[_diario.idx])) {
        novo_registro(&_diario);
      }
      break;
    case 'e':
      diario_entrada = escolher_data(str_data, &_diario, anteriores, ant_qtde);
      entrada(str_data, diario_entrada);
      break;
    case 's':
      status(&(_diario.registros)[_diario.idx]);
      break;
    case 'l':
      imprimir_log(&_diario, anteriores, ant_qtde);
      break;
    case 'q':
      sair = 1;
      break;
    default:
      puts("Comando inválido");
    case 'h': /* FALLTHROUGH */
      puts("\
`t` para iniciar ou parar a contagem de um registro\n\
`e` para entrar com um registro manualmente\n\
`c` para corrigir registros não completos\n\
`s` para mostrar o status do atual registro\n\
`l` para mostrar o log dos regisros mensais\n\
`h` para mostrar esta ajuda\n\
`q` para sair");
    }
  }
  free(_diario.registros);
  for (int i = 0; i < ant_qtde; i += 1) {
    free(anteriores[i].registros);
  }
  return 0;
}
