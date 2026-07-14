#include "TDMScoreboardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TDMGameState.h"
#include "TDMPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMWidget, Log, All);

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace {

/** Create a TextBlock with sensible defaults and add it to a VerticalBox. */
UTextBlock* MakeText(UWidgetTree* WTree, UVerticalBox* Parent,
                     const FString& Label, float FontSize = 18.f,
                     FLinearColor Color = FLinearColor::White) {
  UTextBlock* TB =
      WTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
  TB->SetText(FText::FromString(Label));

  FSlateFontInfo Font = TB->GetFont();
  Font.Size = FontSize;
  TB->SetFont(Font);
  TB->SetColorAndOpacity(FSlateColor(Color));

  UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(TB);
  Slot->SetHorizontalAlignment(HAlign_Center);
  Slot->SetPadding(FMargin(0.f, 2.f));
  return TB;
}

/** Create a TextBlock and add it to a HorizontalBox with Fill sizing. */
UTextBlock* MakeHText(UWidgetTree* WTree, UHorizontalBox* Parent,
                      const FString& Label, float FontSize = 16.f,
                      FLinearColor Color = FLinearColor::White) {
  UTextBlock* TB =
      WTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
  TB->SetText(FText::FromString(Label));

  FSlateFontInfo Font = TB->GetFont();
  Font.Size = FontSize;
  TB->SetFont(Font);
  TB->SetColorAndOpacity(FSlateColor(Color));

  UHorizontalBoxSlot* Slot = Parent->AddChildToHorizontalBox(TB);
  FSlateChildSize Fill;
  Fill.SizeRule = ESlateSizeRule::Fill;
  Fill.Value = 1.f;
  Slot->SetSize(Fill);
  Slot->SetHorizontalAlignment(HAlign_Center);
  return TB;
}

} // namespace

// ─── NativeConstruct ─────────────────────────────────────────────────────────

void UTDMScoreboardWidget::NativeConstruct() {
  Super::NativeConstruct();

  UE_LOG(LogTDMWidget, Log, TEXT("[Scoreboard] NativeConstruct — building layout"));
  BuildLayout();

  // Initial population from whatever state is available now.
  RefreshAll();
}

// ─── BuildLayout ─────────────────────────────────────────────────────────────

void UTDMScoreboardWidget::BuildLayout() {
  UWidgetTree* WTree = WidgetTree;
  if (!WTree) {
    UE_LOG(LogTDMWidget, Warning, TEXT("[Scoreboard] BuildLayout: WidgetTree is null!"));
    return;
  }

  // ── Root: Canvas ──────────────────────────────────────────────────────────
  UCanvasPanel* Root =
      WTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
  WTree->RootWidget = Root;

  // ── Outer SizeBox (anchored top-right, 400×300) ───────────────────────────
  USizeBox* SizeBox =
      WTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox"));
  SizeBox->SetWidthOverride(420.f);
  SizeBox->SetHeightOverride(320.f);

  UCanvasPanelSlot* SBSlot = Root->AddChildToCanvas(SizeBox);
  SBSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f)); // top-right
  SBSlot->SetAlignment(FVector2D(1.f, 0.f));
  SBSlot->SetPosition(FVector2D(-10.f, 10.f));
  SBSlot->SetSize(FVector2D(420.f, 320.f));
  SBSlot->SetAutoSize(false);

  // ── Background border ─────────────────────────────────────────────────────
  UBorder* BG =
      WTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
  BG->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.72f));
  BG->SetPadding(FMargin(10.f));
  SizeBox->SetContent(BG);

  // ── Inner VerticalBox ─────────────────────────────────────────────────────
  UVerticalBox* VBox =
      WTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
  BG->SetContent(VBox);

  // Title
  MakeText(WTree, VBox, TEXT("TEAM DEATHMATCH"), 20.f, FLinearColor::Yellow);

  // Separator (empty row)
  MakeText(WTree, VBox, TEXT(""), 4.f);

  // Team scores row
  UHorizontalBox* ScoreRow =
      WTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ScoreRow"));
  UVerticalBoxSlot* ScoreRowSlot = VBox->AddChildToVerticalBox(ScoreRow);
  ScoreRowSlot->SetHorizontalAlignment(HAlign_Fill);
  ScoreRowSlot->SetPadding(FMargin(0.f, 2.f));

  Text_Team0Score =
      MakeHText(WTree, ScoreRow, TEXT("Team 1: 0"), 22.f, FLinearColor(0.3f, 0.6f, 1.f));
  Text_Team1Score =
      MakeHText(WTree, ScoreRow, TEXT("Team 2: 0"), 22.f, FLinearColor(1.f, 0.4f, 0.3f));

  // Timer
  Text_Timer = MakeText(WTree, VBox, TEXT("Time: 05:00"), 18.f, FLinearColor::White);

  // Separator
  MakeText(WTree, VBox, TEXT("─────────────────"), 10.f, FLinearColor(0.4f, 0.4f, 0.4f));

  // Column header row
  UHorizontalBox* Header =
      WTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
  UVerticalBoxSlot* HeaderSlot = VBox->AddChildToVerticalBox(Header);
  HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
  HeaderSlot->SetPadding(FMargin(0.f, 2.f));

  MakeHText(WTree, Header, TEXT("Player"), 14.f, FLinearColor(0.7f, 0.7f, 0.7f));
  MakeHText(WTree, Header, TEXT("Team"), 14.f, FLinearColor(0.7f, 0.7f, 0.7f));
  MakeHText(WTree, Header, TEXT("K"), 14.f, FLinearColor(0.7f, 0.7f, 0.7f));
  MakeHText(WTree, Header, TEXT("D"), 14.f, FLinearColor(0.7f, 0.7f, 0.7f));

  // Scrollable player list
  UScrollBox* Scroll =
      WTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ScrollBox"));

  Box_PlayerRows =
      WTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerRows"));
  Scroll->AddChild(Box_PlayerRows);

  UVerticalBoxSlot* ScrollSlot = VBox->AddChildToVerticalBox(Scroll);
  FSlateChildSize Fill;
  Fill.SizeRule = ESlateSizeRule::Fill;
  ScrollSlot->SetSize(Fill);

  UE_LOG(LogTDMWidget, Log, TEXT("[Scoreboard] Layout built successfully"));
}

// ─── RefreshAll ──────────────────────────────────────────────────────────────

void UTDMScoreboardWidget::RefreshAll() {
  ATDMGameState* GS =
      GetWorld() ? GetWorld()->GetGameState<ATDMGameState>() : nullptr;

  if (!GS) {
    UE_LOG(LogTDMWidget, Warning,
           TEXT("[Scoreboard] RefreshAll called but GameState not available yet"));
    return;
  }

  // ── Team scores ───────────────────────────────────────────────────────────
  const int32 S0 = GS->TeamScores.IsValidIndex(0) ? GS->TeamScores[0] : 0;
  const int32 S1 = GS->TeamScores.IsValidIndex(1) ? GS->TeamScores[1] : 0;

  if (Text_Team0Score)
    Text_Team0Score->SetText(
        FText::FromString(FString::Printf(TEXT("Team 1: %d"), S0)));

  if (Text_Team1Score)
    Text_Team1Score->SetText(
        FText::FromString(FString::Printf(TEXT("Team 2: %d"), S1)));

  UE_LOG(LogTDMWidget, Log,
         TEXT("[Scoreboard] RefreshAll — Scores: T1=%d T2=%d  Time=%.0f"),
         S0, S1, GS->RemainingMatchTime);

  // ── Timer ─────────────────────────────────────────────────────────────────
  const float T = GS->RemainingMatchTime;
  const int32 Mins = FMath::FloorToInt(T / 60.f);
  const int32 Secs = FMath::FloorToInt(T) % 60;
  if (Text_Timer)
    Text_Timer->SetText(FText::FromString(
        FString::Printf(TEXT("Time: %02d:%02d"), Mins, Secs)));

  // ── Player rows ───────────────────────────────────────────────────────────
  if (!Box_PlayerRows)
    return;

  Box_PlayerRows->ClearChildren();

  for (APlayerState* PS : GS->PlayerArray) {
    if (const ATDMPlayerState* TPS = Cast<ATDMPlayerState>(PS)) {
      FTDMPlayerRow Row;
      Row.PlayerName = TPS->GetPlayerName();
      Row.TeamId     = TPS->TeamId;
      Row.Kills      = TPS->Kills;
      Row.Deaths     = TPS->Deaths;
      AddPlayerRow(Row);
    }
  }
}

// ─── AddPlayerRow ─────────────────────────────────────────────────────────────

void UTDMScoreboardWidget::AddPlayerRow(const FTDMPlayerRow& Row) {
  if (!Box_PlayerRows || !WidgetTree)
    return;

  UHorizontalBox* HBox =
      WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

  auto AddCell = [&](const FString& Txt, FLinearColor Col = FLinearColor::White) {
    MakeHText(WidgetTree, HBox, Txt, 15.f, Col);
  };

  // Team colour tint
  const FLinearColor TeamColor = (Row.TeamId == 0)
      ? FLinearColor(0.5f, 0.8f, 1.f)
      : FLinearColor(1.f, 0.6f, 0.5f);

  AddCell(Row.PlayerName.IsEmpty() ? TEXT("(player)") : Row.PlayerName, TeamColor);
  AddCell(FString::Printf(TEXT("%d"), Row.TeamId + 1), TeamColor);
  AddCell(FString::Printf(TEXT("%d"), Row.Kills),   FLinearColor(0.4f, 1.f, 0.4f));
  AddCell(FString::Printf(TEXT("%d"), Row.Deaths),  FLinearColor(1.f, 0.5f, 0.5f));

  UVerticalBoxSlot* Slot = Box_PlayerRows->AddChildToVerticalBox(HBox);
  Slot->SetHorizontalAlignment(HAlign_Fill);
  Slot->SetPadding(FMargin(0.f, 1.f));
}
