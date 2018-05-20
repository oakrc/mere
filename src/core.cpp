#ifndef MERE_MATH_H
#define MERE_MATH_H

#include "config.hpp"

#include <QFile>
#include <vector>

#if T_GUI
#include <QMessageBox>
#include <QWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#endif

#include "tokenizer.h"
#include "interpreter.h"
#include "parser.h"
#include "astprinter.hpp"
#include "natives.h"

using namespace mere;
Interpreter* Core::interpreter = nullptr;

std::vector<Core::Error> Core::errors{};

void Core::init_once(){
	LFn;
	if (!QMetaType::registerComparators<Object>()){
#if T_GUI
		QMessageBox::critical(nullptr,"Fatal Internal Failure","Failed to register Object comparators.");
#else
		std::cerr << "  > Fatal Internal Error: Failed to register Object comparators\n";
#endif
		Logp("Failed registering comparators.");
		abort();
	}
	interpreter = new Interpreter();
	LVd;
}

bool Core::run(const TString& src, bool show_tok, bool show_syn){
	if (src.isEmpty())
		return false;
	Stmts stmts;
	{
		Tokens tokens = Tokenizer(src).scan_tokens();
		if (show_tok){
			TString str = "";
			int size = tokens.size();
			for (int i = 0; i != size; i++){
				str.append(tokens[i].to_string());
			}
#if T_GUI
			QMessageBox::information(nullptr,"",str);
#else
			std::cout << "  > " << str.toStdString() << std::endl;
#endif
		}
		if (errors.size()){
			show_errors();
			errors.clear();
			return false;
		}
		stmts = Parser(tokens).parse();
	}

	if (errors.size()){
		show_errors();
		errors.clear();
		return false;
	}

	if (show_syn){
		std::cout << "  > AST Printer broken.\n";
#if T_GUI && AST_PRINTER_FIXED
		TString str = ASTPrinter(stmts).AST();
		QWidget* wnd = new QWidget();
		QTextEdit* edt = new QTextEdit();
		QVBoxLayout* layout = new QVBoxLayout();

		QFont font;

		font.setFamily("Courier");
		font.setStyleHint(QFont::Monospace);
		font.setFixedPitch(true);
		font.setPointSize(16);
		edt->setText(str);
		edt->setFont(font);
		edt->setTabStopWidth(4*QFontMetrics(font).width(" "));
		layout->addWidget(edt);
		wnd->setLayout(layout);
		wnd->setAttribute(Qt::WA_DeleteOnClose);
		wnd->show();
#elif AST_PRINTER_FIXED
		std::cout << "  > " + ASTPrinter(stmts).AST().toStdString() + "\n";
#endif
	}
	auto res = interpreter->interpret(stmts);

	errors.clear();
	Logp("End of run");
	return res;
}

bool Core::run_file(QFile & file){
	return run(file.readAll());
}

void Core::error(int ln, const TString& msg){
	report(ln, "", TString("Error: ") + msg);
}

void Core::error(const TString & msg){
	errors.push_back(Error(-1,msg));
#if T_GUI
	QMessageBox::critical(nullptr, "Error", msg);
#else
	std::cout << "  > " << msg.toStdString() << "\n";
#endif
}

void Core::error(const Token& tok, const TString& msg){
	if (tok.ty == Tok::eof) {
		report(tok.ln, "at end", msg);
	}
	else {
		report(tok.ln, (TString)"at '" + tok.lexeme + "'", msg);
	}
}

void Core::report(int ln, const TString& loc, const TString& msg, bool is_idx){
	std::cout << "  > ERROR generated.\n";
	if (is_idx)
		ln++;
	TString p = TString("[Ln ").append(TString::number(ln))
				.append("] ").append(loc)
				.append(": ").append(msg);
	if (!p.endsWith('.'))
		p.append(".");
	errors.push_back(Error(ln,p));
#if T_GUI
	QMessageBox::critical(nullptr,"Error",p);
#endif
}

void Core::runtime_error(const RuntimeError &re){
	TString str = "";
	str.append("[Ln ").append(TString::number(re.tok.ln+1)).append("] ").append(re.msg);
	errors.push_back(Error(re.tok.ln+1,re.msg));
#if T_GUI
	QMessageBox::critical(nullptr,"Runtime Error",str);
#else
	std::cout << "  > Runtime Error: " << re.msg.toStdString() + "\n";
#endif
}

void Core::show_errors(){
	std::cout << "  > Errors";
	if (!errors.size()){
#if T_GUI
		QMessageBox::information(nullptr,"Info","0 error recorded.");
#else
		std::cout << "    > 0 error recorded.\n";
#endif
	}

	TString error_text = "\n";
	for (int i = 0; i != errors.size(); i++){
		error_text.append(TString("    > ") + errors.at(i).msg);
		error_text.append("\n");
	}
#if T_GUI
	QWidget* wind = new QWidget();
	QTextEdit* edt = new QTextEdit();
	QVBoxLayout* vlay = new QVBoxLayout();

	edt->setPlainText(error_text);
	vlay->addWidget(edt);
	wind->setLayout(vlay);
	wind->setAttribute(Qt::WA_DeleteOnClose);
	wind->show();
#else
	std::cout << error_text.toStdString() << std::endl;
#endif
}

void Core::reset_intp(){
	interpreter->reset();
}


#endif // MERE_MATH_H