Convimg (https://github.com/mateoconlechuga/convimg) is used for converting images into 
series of bytes. 

YAML configs used:

```yaml
converts:
  - name: images
    palette: xlibc
    width-and-height: true
    images:
      - image.png

outputs:
  - type: c   
    include-file: gfx.h 
    converts:
      - images
```