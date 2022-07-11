# Biblioteca NBR14522

Este repositório implementa a comunicação convencional leitor-medidor contida na
norma ABNT NBR 14522 do ponto de vista do leitor.

O propósito desta biblioteca é fornecer uma implementação portável e compatível
com sistemas embarcados e microcontrolados bem como com qualquer outro sistema
de propósito geral.

A classe genérica **LeitorFSM** implementa o leitor da comunicação convencional.
Tal classe tem duas dependências (ou policies):

1. **TimerPolicy**: interface que define verificação de alarmes/timeout
2. **SerialPolicy**: interface que define transmissão e recepção de dados de uma
   porta serial (ou qualquer outra porta, desde que respeite essa interface)

Assim, para extender essa biblioteca para outros microcontroladores, ambientes
e/ou sistemas, deve-se implementar as duas dependências/policies e passá-la como
parâmetro à classe genérica **LeitorFSM**. Novas policies de timer e serial
podem ser adicionadas às pastas `{include,src}/{serial,timer}/` e aplicações à
pasta `app/`. Respectivos ajustes ao arquivo `CMakeLists.txt` serão necessários
para compilar a nova aplicação. Além disso, sugere-se adicionar um arquivo de
toolchain do cmake na pasta `cmake/` para compilar para outra plataforma. O
arquivo `cmake/TC-raspberry.cmake` contém um exemplo funcional e testado de
toolchain (Leia os comentários do arquivo `cmake/TC-raspberry.cmake` para
orientações sobre arquivos toolchains do cmake).

Atualmente o repositório contém somente policies para aplicações Windows e
Unix-like. Veja a aplicação `app/leitor-cli` para um exemplo funcional e testado
em um Raspberry.

# Como compilar

# Como testar

# Outros repositórios e/ou projetos

A estrutura e organização de arquivos e do sistema de compilação CMake foram
baseados no repositório https://github.com/bsamseth/cpp-project

Os arquivos `src/serial/win_unix/serialib.cpp` e `include/serial/serialib.h`
foram pegos do repositório https://github.com/imabot2/serialib para a leitura da
porta serial no Windows e Unix-like.

# Licença

Veja o arquivo `LICENSE`.