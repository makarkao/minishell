/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   60-ft_cd_helper.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: melayyad <melayyad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/28 10:57:25 by melayyad          #+#    #+#             */
/*   Updated: 2025/07/13 21:58:17 by melayyad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../minishell.h"

int	error_messages(int type, char *message, char *file)
{
	ft_putstr_fd(2, "minishell");
	ft_putstr_fd(2, ": cd: ");
	if (type == 1)
		ft_putstr_fd(2, message);
	else if (type == 2)
	{
		ft_putstr_fd(2, file);
		write(2, ": ", 2);
		ft_putstr_fd(2, message);
	}
	write(2, "\n", 1);
	return (1);
}

void	print_error(void)
{
	perror("cd: error retrieving current directory:"
		"getcwd: cannot access parent directories: ");
}

int	update_pwd(t_shelldata *shelldata)
{
	char	*new_pwd;
	char	*env_name;
	t_env	*env_pwd;

	new_pwd = getcwd(NULL, 0);
	if (!new_pwd)
		return (print_error(), 1);
	else
	{
		if (shelldata->cwd)
			(free(shelldata->cwd), shelldata->cwd = NULL);
		shelldata->cwd = ft_strdup(new_pwd);
		if (!shelldata->cwd)
			return (shelldata->state = -2, 1);
	}
	env_pwd = get_env_str("PWD", shelldata->env);
	if (env_pwd)
		return (free(env_pwd->value), env_pwd->value = new_pwd, 0);
	env_name = ft_strdup("PWD");
	if (!env_name)
		return (free(new_pwd), shelldata->state = -2, 1);
	add_back_env(shelldata, env_name, new_pwd, 0);
	if (shelldata->state < 0)
		return (free(new_pwd), free(env_name), 1);
	return (0);
}

int	update_oldpwd(t_shelldata *shelldata)
{
	t_env	*env_opwd;
	char	*env_name;
	char	*pwd_value;
	char	*oldpwd_nvalue;

	pwd_value = extract_variable_value("PWD", shelldata->env);
	if (pwd_value)
	{
		oldpwd_nvalue = ft_strdup(pwd_value);
		if (!oldpwd_nvalue)
			return (shelldata->state = -2, 1);
	}
	else
		oldpwd_nvalue = NULL;
	env_opwd = get_env_str("OLDPWD", shelldata->env);
	if (env_opwd)
		return (free(env_opwd->value), env_opwd->value = oldpwd_nvalue, 0);
	env_name = ft_strdup("OLDPWD");
	if (!env_name)
		return (free(oldpwd_nvalue), shelldata->state = -2, 1);
	add_back_env(shelldata, env_name, oldpwd_nvalue, 0);
	if (shelldata->state < 0)
		return (free(oldpwd_nvalue), free(env_name), 1);
	return (0);
}
