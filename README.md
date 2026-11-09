# Genshin_Clone

언리얼 엔진 기반 원신 모작 프로젝트.

## 폴더 구조

```text
Genshin_Clone/
├─ Config/                     # 프로젝트 설정
├─ Content/
│  ├─ Game/                    # 직접 제작한 블루프린트, 맵, UI
│  └─ ExternalAsset/           # 외부 에셋 (Git 제외)
├─ Source/
│  ├─ Genshin_Clone/
│  │  ├─ Public/               # 공개 헤더
│  │  ├─ Private/              # 구현 및 내부 헤더
│  │  └─ Genshin_Clone.Build.cs
│  ├─ Genshin_Clone.Target.cs
│  └─ Genshin_CloneEditor.Target.cs
├─ Genshin_Clone.uproject
├─ Merge-UnrealFilters.ps1     # VS 필터 정리
└─ Merge-UnrealFilters.bat     # 필터 정리 실행
```
