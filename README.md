# 원신 모작

## 🎮 프로젝트 소개

C++과 Unreal Engine을 활용해 개발 중인 원신 모작 개인 프로젝트입니다.

## 🛠 개발 정보

| 항목 | 내용 |
| --- | --- |
| 개발 형태 | 개인 프로젝트 |
| 개발 인원 | 1명 |
| 개발 상태 | 개발 중 |
| 엔진 버전 | Unreal Engine 5.8 |
| 개발 환경 | Windows, Unreal Editor, C++ 개발 환경 |
| 버전 관리 | Git, GitHub |

## 💻 기술 스택

| 분류 | 기술 |
| --- | --- |
| 언어 | C++ |
| 게임 엔진 | Unreal Engine 5.8 |
| 블루프린트 | Blueprint, Animation Blueprint |

## 📌 주요 개발 내용

- AI 협업 워크플로우 구축
- 플레이어 이동·질주·대시 및 애니메이션 전환
- 원본·Mirror 애니메이션 선택
- 연속 대시 판정과 재입력 제한·쿨타임

## 📁 저장소 구조

```text
Genshin_Clone/
├─ Config/                     # 프로젝트 및 입력 설정
├─ Content/
│  ├─ Game/
│  │  ├─ Characters/           # 플레이어 Blueprint와 Animation Blueprint
│  │  ├─ GameModes/            # 게임 모드
│  │  ├─ Input/                # Input Action과 Input Mapping Context
│  │  └─ Maps/                 # 테스트 맵
│  └─ ExternalAsset/           # 외부 캐릭터·애니메이션 에셋 (Git 제외)
├─ Source/
│  ├─ Genshin_Clone/
│  │  ├─ Public/               # 공개 헤더와 애니메이션 데이터 구조
│  │  ├─ Private/              # 캐릭터·입력·대시·애니메이션 구현
│  │  └─ Genshin_Clone.Build.cs
│  ├─ Genshin_Clone.Target.cs
│  └─ Genshin_CloneEditor.Target.cs
├─ Genshin_Clone.uproject
├─ Merge-UnrealFilters.ps1     # Visual Studio 필터 정리
└─ Merge-UnrealFilters.bat     # 필터 정리 실행
```

## 🔧 실행 안내

1. Unreal Engine 5.8과 Unreal C++ 프로젝트를 빌드할 수 있는 개발 환경을 준비합니다.
2. 프로젝트에서 참조하는 외부 캐릭터·애니메이션 에셋을 `Content/ExternalAsset/` 아래의 참조 경로에 맞게 준비합니다.
3. `Genshin_Clone.uproject`를 열고, 필요한 경우 프로젝트 파일 생성과 C++ 모듈 빌드를 진행합니다.
4. `Content/Game/Maps/TestMap`을 열어 플레이합니다.

외부 에셋은 저장소에 포함되어 있지 않으므로, 해당 에셋이 준비된 환경에서 실행해야 합니다.

## ℹ️ 안내

- 본 프로젝트는 「원신」을 모작한 비상업적 학습·포트폴리오 프로젝트입니다.
- 원작 게임과 외부 라이브러리·에셋의 권리는 각 권리자에게 있습니다.
