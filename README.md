# C++ Search Engine

## Сборка (без MongoDB, через sample.tsv)
> Требования: CMake 3.20+, компилятор с C++20, Интернет

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Запуск CLI
```bash
./build/search_engine --cli --sample data/sample.tsv
```

### Запуск Web
```bash
./build/search_engine --web --port 8080 --sample data/sample.tsv
# открой http://localhost:8080
```

## Сборка с MongoDB
1) Установить `mongocxx` и `bsoncxx`
2) Собрать:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_MONGODB=ON
cmake --build build -j
```

### Запуск с MongoDB
```bash
./build/search_engine --mongo \
  --mongo-uri mongodb://localhost:27017 \
  --mongo-db YOUR_DB \
  --mongo-col YOUR_COLLECTION \
  --cli
```

## Экспорт Zipf CSV
```bash
./build/search_engine --cli --sample data/sample.tsv --export-zipf --zipf-path data/zipf.csv
```

Построение графика:
```bash
python3 scripts/zipf_plot.py data/zipf.csv
```

## Тестовые запросы
```
simple AND test
(hello OR world) AND NOT goodbye
"web development" AND javascript
data AND (science OR analysis)
python AND NOT java
```

## Примечания по фразовому поиску
Фразы `"exact phrase"` проверяются по нормализованному тексту документа:
- lower-case
- все не буквенно-цифровые символы заменяются на пробел
- пробелы схлопываются
