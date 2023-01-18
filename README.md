# Relógio ponto
## Como usar
> ./relogio

O programa é interativo pela CLI, para ver os comandos disponíveis digite `h`

    `t` para iniciar ou parar a contagem de um registro
    `s` para mostrar o status do atual registro
    `l` para mostrar o log dos regisros mensais
    `h` para mostrar esta ajuda
    `q` para sair

## Formatação do log
O programa salva os registros em um arquivo `log.txt`, a formatação é bem simples e segue o padrão:

    Data/Mês/Ano
    Horário_de_início;Horário_de_fim
    Horário_de_início;Horário_de_fim
    ...
    Data/Mês/Ano
    ...
