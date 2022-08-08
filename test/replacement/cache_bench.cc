#include "mmkv/replacement/cache_interface.h"
#include "mmkv/replacement/lru_cache.h"
#include "mmkv/replacement/mru_cache.h"

using namespace mmkv::replacement;

#define MULTIPILER 10
#define RANGE 10000
#define PASS 10

static inline auto cache_hit_bench(CacheInterface<int> *cache) -> void {
  auto range = MULTIPILER;
  for (;;) {
    int hit = 0;
    for (int c = 0; c < PASS; ++c) {
      for (int i = 0; i < range; ++i) {
        if (cache->Search(i))
          hit++;
        cache->UpdateEntry(i);
      }
    }
    
    cache->Clear();
    printf("Hit count %9d for range %9d in %d pass\n", hit, range, PASS);
    range *= MULTIPILER;
    if (range > RANGE) break;
  }
}

int main() {
  std::unique_ptr<LruCache<int>> lru_cache(new LruCache<int>(RANGE / 10));
  printf("LRU Replacement\n");
  cache_hit_bench(lru_cache.get());
  std::unique_ptr<MruCache<int>> mru_cache(new MruCache<int>(RANGE / 10));
  printf("MRU Replacement\n");
  cache_hit_bench(mru_cache.get());

}
